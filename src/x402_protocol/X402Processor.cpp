#include "MachinePayCommon.h"
#include "X402Processor.h"
#include "MachinePayApp.h"
#include "IResponseSender.h"
#include "BackendConnection.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ServerConfig.h"
#include "payment/datastructures/PaymentPayload.h"
#include "payment/datastructures/PaymentRequirements.h"
#include "payment/datastructures/PaymentRequiredResponse.h"
#include "url/URLUtils.h"

#include <boost/beast/core/detail/base64.hpp>
#include <folly/json.h>



X402Processor::X402Processor(MachinePayApp &app, ptr<IResponseSender> &responseSender)
    : app_(app), responseSender_(responseSender) {
    config_ = app_.configManager()->latestConfig();
}


bool X402Processor::reply402IfNoPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage> &req) {
    try {
        if (req->getHeaders().exists("X-PAYMENT"))
            return false;

        reply402PaymentRequired();
    } catch (std::exception &e) {
        spdlog::error("[hasPaymentHeader] Exception while checking X-PAYMENT header: {}", e.what());
    }

    return true;
}



/**
| x402 Error       | HTTP Status | Description                                     |
| ---------------- | ----------- | ----------------------------------------------- |
| Payment Required | 402         | Payment needed to access resource               |
| Invalid Payment  | 400         | Malformed payment payload or requirements       |
| Payment Failed   | 402         | Payment verification or settlement failed       |
| Server Error     | 500         | Internal server error during payment processing |
| Success          | 200         | Payment verified and settled successfully       |
*/

void X402Processor::reply200Success(const std::string &settlementInfo,
                                    std::string& proxyBody) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "text/plain"},
        {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    sendResponse({200, "OK"}, headers, proxyBody);
    state_ = State::SUCCESS_RESOURCE_SENT;
}


void X402Processor::reply402PaymentRequired() {
    try {
        CHECK_STATE(resource_);

        auto paymentRequirements = PaymentRequiredResponse::getPaymentRequiredResponseAsString(organization(),
            resource(), config());

        std::vector<std::pair<std::string, std::string> > headers = {
            {"Content-Type", "application/json"}
        };

        sendResponse({402, "Payment Required"}, headers, paymentRequirements);
        state_ = State::SUCCESS_PAYMENT_REQUIRED_SENT;
    } catch (std::exception &e) {
        RETHROW_NESTED;
    }
}

void X402Processor::reply400BadRequest1(const std::string &message) {

    CHECK_STATE(resource_);
    auto paymentRequirements = PaymentRequiredResponse::getPaymentRequiredResponseAsString(organization(),
        resource(), config());


    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "application/json"}
    };

    nlohmann::json j;
    j["error"] = message;
    auto body = j.dump();
    sendResponse({400, "Bad Request"}, headers, body);
    state_ = State::ERROR_SENT;
}

void X402Processor::reply400InvalidPayment(const std::string &message) {

    auto paymentRequirements = PaymentRequiredResponse::getPaymentRequiredResponseAsString(organization(),
        resource(), config());

    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "application/json"}
    };


    sendResponse({400, "Invalid Payment"}, headers, message);
    state_ = State::ERROR_SENT;
}


void X402Processor::reply500InternalError(const std::string &message) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "application/json"}
    };

    nlohmann::json j;
    j["error"] = message;
    auto body = j.dump();


    sendResponse({500, "Server Error"}, headers, body);
    state_ = State::ERROR_SENT;
}

void X402Processor::reply502BadGateway(const std::string &message) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "application/json"}
    };


    nlohmann::json j;
    j["error"] = message;
    auto body = j.dump();


    sendResponse({502, "Bad Gateway"}, headers, body);
    state_ = State::ERROR_SENT;
}




void X402Processor::sendResponse(
    const std::pair<uint16_t, std::string> &statusAndMessage,
    const std::vector<std::pair<std::string, std::string> > &headers,
    const std::string &body ) {
    if (state_ == State::ERROR_SENT)
    {
        spdlog::info("Attempted to send response after error response already sent.");
        return;
    }

    if (state_ == State::SUCCESS_RESOURCE_SENT)
    {
        spdlog::info("Attempted to send response after resource already sent.");
        return;
    }

    if (state_ == State::SUCCESS_PAYMENT_REQUIRED_SENT)
    {
        spdlog::info("Attempted to send response after payment required already sent.");
        return;
    }


    CHECK_STATE(responseSender_);
    try
    {
        responseSender_->sendResponse(statusAndMessage, headers, body);
    } catch (std::exception &e)
    {
        spdlog::error("Exception while sending response: {}", e.what());
        // nothing can be done so we consider response as sent
    }
}


bool X402Processor::validateAndExtractSubDomainName(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto domainName = reqHeaders->getHeaders().getSingleOrEmpty("host");
    // Remove port if present (e.g., example.com:8080 -> example.com)


    if (domainName.empty()) {
        reply400BadRequest1("Missing Host header");
        return false;
    }


    if (URLUtils::isIpAddress(domainName)) {
        reply400BadRequest1(
            "Unknown host: " + domainName + ". You need to access MachinePay using a hostname, not an IP address. "
            "Please use a valid hostname to access this service.");
        return false;
    }

    auto colonPos = domainName.rfind(':');
    if (colonPos != std::string::npos) {
        domainName = domainName.substr(0, colonPos);
    }

    // check for IP address again

    if (URLUtils::isIpAddress(domainName)) {
        reply400BadRequest1(
            "Unknown host: " + domainName + ". You need to access MachinePay using a hostname, not an IP address. "
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    if (!URLUtils::isDomainName(domainName)) {
        reply400BadRequest1(
            "Invalid host name: " + domainName + "."
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    auto hostName = config_->server()->hostName();

    if (domainName == hostName) {
        subDomainName_ = "";
    } else if (domainName.ends_with("." + hostName)) {
        subDomainName_ = domainName.substr(0, domainName.size() - hostName.size() - 1);
    } else {
        reply400BadRequest1("Unknown host: " + domainName + " "
                           "Please use a valid hostname specified in machinepay config "
                           "(like localhost or xyz.com) to access this service.");
        return false;
    }

    return true;
}

bool X402Processor::validateAndDecodePath(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto path = reqHeaders->getPath();
    std::string errorMessage;
    if (!URLUtils::decodePath(path, decodedPath_, errorMessage)) {
        reply400BadRequest1(errorMessage);
        return false;
    }
    return true;
}

bool X402Processor::matchOrganization() {
    organization_ = config()->getOrganizationBySubdomainName(subDomainName_);

    if (!organization_) {
        reply400BadRequest1("Unknown subdomain  " + subDomainName_ + "." + config_->server()->hostName() +
                           " Please use a valid subdomain specified in machinepay config "
                           "(like localhost or xyz.com) to access this service.");
        return false;
    }
    return true;
}

bool X402Processor::validateMethod(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto method = reqHeaders->getMethod();

    if (!method.has_value()) {
        reply400BadRequest1("Missing HTTP method");
    }

    method_ = method.value();

    if (method_ != proxygen::HTTPMethod::GET &&
        method_ != proxygen::HTTPMethod::POST) {
        reply400BadRequest1("Unsupported HTTP method. Only GET and POST are supported" +
                           reqHeaders->getMethodString());
        return false;
    }
    return true;
}

void X402Processor::onRequestStart(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders)
    noexcept {
    try {
        CHECK_STATE(reqHeaders);

        if (!validateMethod(reqHeaders))
            return;

        if (!validateAndExtractSubDomainName(reqHeaders))
            return;

        if (!matchOrganization())
            return;

        if (!validateAndDecodePath(reqHeaders))
            return;
    } catch (std::exception &e) {
        spdlog::critical("onRequestStart exception");
        printNestedException(e);
        reply500InternalError("Could not process x402 request start.");
    };
}

bool X402Processor::proxyResponseToBackEnd(std::string& responseBody) {
    std::string errorMessage;
    bool success = BackendConnection::proxyToBackEnd(responseBody, errorMessage);
    if (!success) {
        reply502BadGateway(errorMessage.empty() ? "Failed to fetch content from upstream service." : errorMessage);
        return false;
    }
    return true;
}

void X402Processor::replyToClientWithError(const HttpError& httpError) {
    auto httpErrorMessage = httpError.message();
    switch (httpError.type()) {
        case ErrorType::ERR_BAD_REQUEST:
            reply400InvalidPayment(httpErrorMessage);
            break;
        case ErrorType::ERR_INTERNAL_SERVER_ERROR:
            reply500InternalError(httpErrorMessage);
            break;
        case ErrorType::ERR_BAD_GATEWAY:
            reply502BadGateway(httpErrorMessage);
            break;
        default:
            // cant happen
            CHECK_STATE(false);
    }
}

void X402Processor::onRequestFullyReceived(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                           const string &body) noexcept {
    try {
        if (state_ == State::ERROR_SENT) return;

        CHECK_STATE(organization_);
        resource_ = organization_->getResourceByPath(decodedPath_, method_, body);

        if (!resource_) {
            reply400BadRequest1("Resource not found for path: " + decodedPath_);
            return;
        }

        if (reply402IfNoPaymentHeader(reqHeaders)) {
            return;
        }

        auto result =  app_.paymentManager()->decodeValidateAndSettlePayment(reqHeaders,
             *config(), *resource(), *organization());

        if (holds_alternative<HttpError>(result)) {
            auto error = std::get_if<HttpError>(&result);
            replyToClientWithError(*error);
            return;
        }

        state_ = State::PAYMENT_HEADER_RECEIVED;

        string responseBody;
        if (!proxyResponseToBackEnd(responseBody)) {
            return;
        }

        auto settlementResponse = std::get<SettlementResponse>(result);

        reply200Success(settlementResponse.originalJsonToBase64(), responseBody);
    } catch (std::exception &e) {
        spdlog::critical("onRequestCompletion exception");
        printNestedException(e);
        reply500InternalError("Could not process x402 request.");
    }
}

void X402Processor::onBodySizeIncrease(size_t newSize) {
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024; // 128 KB
    spdlog::info("[onBodySizeIncrease] Request body size increased to {} bytes", newSize);
    if (newSize > MAX_BODY_SIZE) {
        reply400BadRequest1("Request body too large. Maximum allowed is 1MByte. You can increase this limit in "
            "machinepay config if needed.");
    }
}
