#include "X402Processor.h"
#include "BackendConnection.h"
#include "IResponseSender.h"
#include "MachinePayApp.h"
#include "MachinePayCommon.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ServerConfig.h"
#include "payment/datastructures/PaymentPayload.h"
#include "payment/datastructures/PaymentRequiredResponse.h"
#include "payment/datastructures/PaymentRequirements.h"
#include "url/URLUtils.h"


X402Processor::X402Processor(MachinePayApp &app, ptr<IResponseSender> &responseSender)
    : app_(app), responseSender_(responseSender) {
    config_ = app_.configManager()->latestConfig();
}

bool X402Processor::isReplySent() const {
    return state_ == X402ProcessorState::ERROR_SENT ||
           state_ == X402ProcessorState::SUCCESS_REPLY_SENT;
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


bool X402Processor::reply402IfNoPaymentHeader(
    const std::unique_ptr<proxygen::HTTPMessage> &req) {
    try {
        if (req->getHeaders().exists("X-PAYMENT"))
            return false;

        reply402PaymentRequired(std::nullopt);
    } catch (std::exception &e) {
        spdlog::error("[hasPaymentHeader] Exception while checking X-PAYMENT header: {}",
                      e.what());
    }

    return true;
}


void X402Processor::reply502BadGateway(const std::string &message) {
    sendResponse({502, "Bad Gateway"}, STANDARD_HEADERS, message);

    string body = getErrorBody(message);

    sendResponse({502, "Bad Gateway"}, STANDARD_HEADERS, body);
    state_ = X402ProcessorState::ERROR_SENT;
}

void X402Processor::reply200Success(
    const std::string &settlementInfo, std::string &proxiedBody) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "text/plain"}, {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    sendResponse({200, "OK"}, headers, proxiedBody);
    state_ = X402ProcessorState::SUCCESS_REPLY_SENT;
}


void X402Processor::reply402PaymentRequired(
    std::optional<SettlementResponse> errorResponse) {
    try {
        CHECK_STATE(resource_);

        std::optional<string> errorString = std::nullopt;

        std::vector<std::pair<std::string, std::string> > headers;

        if (errorResponse) {
            errorString = errorResponse->errorReason().value_or(
                "Payment required to access resource");
            auto settlementInfo = errorResponse->originalJsonToBase64();
            headers = {
                {"Content-Type", "application/json"},
                {"X-PAYMENT-RESPONSE", settlementInfo}
            };
        } else {
            headers = STANDARD_HEADERS;
        }

        auto paymentRequirements =
                PaymentRequiredResponse::getPaymentRequiredResponseAsString(
                    organization(), resource(), config(), errorString);

        sendResponse({402, "Payment Required"}, headers, paymentRequirements);
        state_ = X402ProcessorState::SUCCESS_REPLY_SENT;
    } catch (std::exception &e) {
        RETHROW_NESTED;
    }
}


void X402Processor::reply400InvalidPayment(const std::string &message) {
    auto paymentRequirements =
            PaymentRequiredResponse::getPaymentRequiredResponseAsString(
                organization(), resource(), config());

    sendResponse({400, "Invalid Payment"}, STANDARD_HEADERS, message);
    state_ = X402ProcessorState::ERROR_SENT;
}

void X402Processor::reply400ResourceNotFound(const std::string &message) {
    auto paymentRequirements =
            PaymentRequiredResponse::getPaymentRequiredResponseAsString(
                organization(), resource(), config());

    sendResponse({400, "Resource Not Found"}, STANDARD_HEADERS, message);
    state_ = X402ProcessorState::ERROR_SENT;
}


std::string X402Processor::getErrorBody(const std::string &message) {
    if (organization_ && resource_ && config_) {
        return PaymentRequiredResponse::getPaymentRequiredResponseAsString(
            organization(), resource(), config(), message);
    } else {
        nlohmann::json j;
        j["error"] = message;
        return j.dump();
    }
}

void X402Processor::reply500InternalError(const std::string &message) {
    string body = getErrorBody(message);
    sendResponse({500, "Server Error"}, STANDARD_HEADERS, body);
    state_ = X402ProcessorState::ERROR_SENT;
}


void X402Processor::sendResponse(
    const std::pair<uint16_t, std::string> &statusAndMessage,
    const std::vector<std::pair<std::string, std::string> > &headers,
    const std::string &body) {
    if (state_ == X402ProcessorState::ERROR_SENT) {
        spdlog::info("Attempted to send response after error response already sent.");
        return;
    }

    if (state_ == X402ProcessorState::SUCCESS_REPLY_SENT) {
        spdlog::info("Attempted to send response after resource already sent.");
        return;
    }


    CHECK_STATE(responseSender_);
    try {
        responseSender_->sendResponse(statusAndMessage, headers, body);
    } catch (std::exception &e) {
        spdlog::error("Exception while sending response: {}", e.what());
        // nothing can be done so we consider response as sent
    }
}


bool X402Processor::validateAndExtractSubDomainName(
    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto domainName = reqHeaders->getHeaders().getSingleOrEmpty("host");
    // Remove port if present (e.g., example.com:8080 -> example.com)

    if (domainName.empty()) {
        reply400InvalidPayment("Missing Host header");
        return false;
    }

    if (URLUtils::isIpAddress(domainName)) {
        reply400InvalidPayment(
            "Unknown host: " + domainName +
            ". You need to access MachinePay using a hostname, not an IP address. "
            "Please use a valid hostname to access this service.");
        return false;
    }

    auto colonPos = domainName.rfind(':');
    if (colonPos != std::string::npos) {
        domainName = domainName.substr(0, colonPos);
    }

    // check for IP address again

    if (URLUtils::isIpAddress(domainName)) {
        reply400InvalidPayment(
            "Unknown host: " + domainName +
            ". You need to access MachinePay using a hostname, not an IP address. "
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    if (!URLUtils::isDomainName(domainName)) {
        reply400InvalidPayment(
            "Invalid host name: " + domainName +
            "."
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
        reply400InvalidPayment(
            "Unknown host: " + domainName +
            " "
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    return true;
}

bool X402Processor::validateAndDecodePath(
    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto path = reqHeaders->getPath();
    std::string errorMessage;
    if (!URLUtils::decodePath(path, decodedPath_, errorMessage)) {
        reply400InvalidPayment(errorMessage);
        return false;
    }
    return true;
}

bool X402Processor::matchOrganization() {
    organization_ = config()->getOrganizationBySubdomainName(subDomainName_);

    if (!organization_) {
        reply400InvalidPayment(
            "Unknown subdomain  " + subDomainName_ + "." + config_->server()->hostName() +
            " Please use a valid subdomain specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }
    return true;
}

bool X402Processor::validateMethod(
    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto method = reqHeaders->getMethod();

    if (!method.has_value()) {
        reply400InvalidPayment("Missing HTTP method");
    }

    method_ = method.value();

    if (method_ != proxygen::HTTPMethod::GET && method_ != proxygen::HTTPMethod::POST
        && method != proxygen::HTTPMethod::HEAD && method != proxygen::HTTPMethod::OPTIONS &&
        method != proxygen::HTTPMethod::PUT) {
        reply400InvalidPayment(
            "Unsupported HTTP method." +
            reqHeaders->getMethodString());
        return false;
    }
    return true;
}

void X402Processor::onRequestStart(
    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) noexcept {
    try {
        CHECK_STATE(reqHeaders);

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
    } catch (...) {
        spdlog::critical("onRequestStart unknown exception");
        reply500InternalError("Could not process x402 request start.");
    };
}


bool X402Processor::proxyResponseToBackEnd(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                           const std::string &requestBody, vector<pair<string, string>> &responseHeaders,
                                           std::string &responseBody) {
    std::string errorMessage;
    bool success = BackendConnection::proxyToBackEnd( url,
        method_, reqHeaders, requestBody, responseHeaders, responseBody, errorMessage);
    if (!success) {
        reply502BadGateway(errorMessage.empty() ? "Failed to fetch content from upstream service." : errorMessage);
        return false;
    }
    return true;
}

void X402Processor::replyToClientWithError(const HttpError &httpError) {
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

void X402Processor::sendSettlementErrorResponse(
    ptr<Authorization> authorization, add_pointer_t<HttpError> const error) {
    if (error->type() == ErrorType::ERR_INTERNAL_SERVER_ERROR) {
        reply500InternalError(error->message());
    } else {
        string payer;
        if (authorization) {
            // extract payer address from authorization
            // if we do not have authorization, we put empty payer
            payer = authorization->from().toHex(PREFIX_0x);
        }
        auto errorSettlementResponse = SettlementResponse::getErrorSettlementResponse(
            *error, config()->network()->name(), payer);
        CHECK_STATE(errorSettlementResponse);
        reply402PaymentRequired(*errorSettlementResponse);
    }
}

void X402Processor::doPassThrough(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, const string &body) {
    try {
        // Validate request method (only GET/POST supported for now)
        if (!validateMethod(reqHeaders)) {
            return;
        }

        // Proxy the request to the backend without any payment checks
        std::string responseBody;

        std::vector<std::pair<std::string, std::string> > responseHeaders;

        string domain;

        if (organization()->subdomain().empty()) {
            domain = config()->server()->hostName();
        } else {
            domain = organization()->subdomain() + config()->server()->hostName();
        }

        auto url = "http://" + domain + "." + reqHeaders->getPath();


        auto result = proxyResponseToBackEnd(url, reqHeaders, body, responseHeaders, responseBody);

        if (!result) {
            return; // proxyResponseToBackEnd already sent an error response
        }

        // For pass-through, just return 200 OK with standard headers and the proxied body
        sendResponse({200, "OK"}, responseHeaders, responseBody);
        state_ = X402ProcessorState::SUCCESS_REPLY_SENT;
    } catch (std::exception &e) {
        spdlog::critical("doPassThrough exception");
        printNestedException(e);
        reply500InternalError("Could not process pass-through request.");
    } catch (...) {
        spdlog::critical("doPassThrough unknown exception");
        reply500InternalError("Could not process pass-through request.");
    }
}

void X402Processor::onRequestFullyReceived(
    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    const string &body) noexcept {
    try {
        if (state_ == X402ProcessorState::ERROR_SENT)
            return;

        CHECK_STATE(organization_);
        resource_ = organization_->getResourceByPath(decodedPath_, method_, body);

        if (!resource_) {
            // resource not found. If is pass through organization, do pass through
            // else reply 400
            if (organization()->passThroughConfig()) {
                doPassThrough(reqHeaders, body);
            } else {
                reply400ResourceNotFound(
                    "Requested resource not found: " + decodedPath_);
            }
            return;
        }


        if (!validateMethod(reqHeaders))
            return;

        if (reply402IfNoPaymentHeader(reqHeaders)) {
            return;
        }

        ptr<Authorization> authorization;

        auto result = app_.paymentManager()->decodePreValidateAndSettleWithFacilitator(
            reqHeaders, *config(), *resource(), *organization(), authorization);

        if (holds_alternative<HttpError>(result)) {
            auto const error = std::get_if<HttpError>(&result);
            sendSettlementErrorResponse(authorization, error);
            return;
        }

        string responseBody;

        vector<pair<string, string>> responseHeaders;


        if (!proxyResponseToBackEnd(resource_->getLocation(), reqHeaders, body,responseHeaders,  responseBody)) {
            return;
        }

        auto settlementResponse = std::get<SettlementResponse>(result);

        reply200Success(settlementResponse.originalJsonToBase64(), responseBody);
    } catch (std::exception &e) {
        spdlog::critical("onRequestCompletion exception");
        printNestedException(e);
        reply500InternalError("Could not process x402 request.");
    } catch (...) {
        spdlog::critical("onRequestCompletion unknown exception");
        reply500InternalError("Could not process x402 request.");
    };
}


void X402Processor::onBodySizeIncrease(size_t newSize) {
    if (state_ == X402ProcessorState::ERROR_SENT)
        return;
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024;
    if (newSize > MAX_BODY_SIZE) {
        reply400InvalidPayment(
            "Request body too large. Maximum allowed is 1MByte. You can increase this "
            "limit in "
            "machinepay config if needed.");
    }
}

const std::vector<std::pair<std::string, std::string> >
X402Processor::STANDARD_HEADERS = {{"Content-Type", "application/json"}};
