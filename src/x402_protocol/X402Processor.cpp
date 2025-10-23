#include "MachinepayCommon.h"
#include "X402Processor.h"
#include "MachinePayApp.h"
#include "IResponseSender.h"
#include <folly/json.h>
#include "BackendConnection.h"
#include "examples/PaymentExamples.h"

#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ServerConfig.h"
#include "payment/X402PaymentRequirements.h"
#include "../payment/PaymentRequirements.h"


X402Processor::X402Processor(MachinePayApp &app, ptr<IResponseSender> &responseSender)
    : app_(app), responseSender_(responseSender) {
    config_ = app_.configManager()->latestConfig();
}

bool X402Processor::hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage> &req, std::string &paymentInfo) {
    try {
        const auto &headerTable = req->getHeaders();
        std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
        if (payment.empty()) return false;
        if (payment == "demo-ok") {
            paymentInfo =
                    R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"SDC\",\"network\":\"base-1net\"})";
            return true;
        }
    } catch (std::exception &e) {
        spdlog::error("Error parsing payment header: {}", e.what());
    }
    return false;
}


void X402Processor::reply200Success(const std::string &settlementInfo,
                                    std::string proxyBody) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "text/plain"},
        {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    sendResponse({200, "OK"}, headers, proxyBody);
    state_ = State::SUCCESS_RESOURCE_PROVIDED;
}




void X402Processor::reply402PaymentRequired() {
    CHECK_STATE(resource_);
    folly::dynamic req = folly::dynamic::object;
    auto paymentRequirements = PaymentRequirements::getPaymentRequirementsAsString(organization(),
        resource(), config());
    req = folly::parseJson(paymentRequirements);
    auto json = folly::toJson(req);
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "application/json"}
    };
    sendResponse({402, "Payment Required"}, headers, json);
    state_ = State::SUCCESS_PAYMENT_REQUIRED_SENT;
}

void X402Processor::reply400BadRequest(const std::string &message) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "text/plain"}
    };
    sendResponse({400, "Bad Request"}, headers, message);
    state_ = State::ERROR;
}


void X402Processor::reply502BadGateway(const std::string &message) {
    std::vector<std::pair<std::string, std::string> > headers = {
        {"Content-Type", "text/plain"}
    };
    sendResponse({502, "Bad Gateway"}, headers, message);
    state_ = State::ERROR;
}


void X402Processor::sendResponse(
    const std::pair<uint16_t, std::string> &statusAndMessage,
    const std::vector<std::pair<std::string, std::string> > &headers,
    const std::string &body = "") {
    CHECK_STATE(state_ != State::ERROR);
    CHECK_STATE(responseSender_);
    responseSender_->sendResponse(statusAndMessage, headers, body);
}




bool X402Processor::validateAndExtractSubDomainName(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto domainName = reqHeaders->getHeaders().getSingleOrEmpty("host");
    // Remove port if present (e.g., example.com:8080 -> example.com)


    if (domainName.empty()) {
        reply400BadRequest("Missing Host header");
        return false;
    }


    if (URLUtils::isIpAddress(domainName)) {
        reply400BadRequest(
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
        reply400BadRequest(
            "Unknown host: " + domainName + ". You need to access MachinePay using a hostname, not an IP address. "
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    if (!URLUtils::isDomainName(domainName)) {
        reply400BadRequest(
            "Invalid host name: " + domainName + "."
            "Please use a valid hostname specified in machinepay config "
            "(like localhost or xyz.com) to access this service.");
        return false;
    }

    auto hostName = config_->server()->hostName();

    if (domainName == hostName) {
        subDomainName_ = "";
    }else if (domainName.ends_with("." + hostName)) {
        subDomainName_ = domainName.substr(0, domainName.size() - hostName.size() - 1);
    } else {
        reply400BadRequest("Unknown host: " + domainName + " "
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
        reply400BadRequest(errorMessage);
        return false;
    }
    return true;
}

bool X402Processor::matchOrganization() {
    organization_ = config()->getOrganizationBySubdomainName(subDomainName_);

    if (!organization_) {
        reply400BadRequest("Unknown subdomain  " + subDomainName_ + "." + config_->server()->hostName() +
                           " Please use a valid subdomain specified in machinepay config "
                           "(like localhost or xyz.com) to access this service.");
        return false;
    }
    return true;
}

bool X402Processor::validateMethod(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    auto method = reqHeaders->getMethod();

    if (!method.has_value() ) {
        reply400BadRequest("Missing HTTP method");
    }

    method_ = method.value();

    if (method_ != proxygen::HTTPMethod::GET &&
        method_ != proxygen::HTTPMethod::POST) {
        reply400BadRequest("Unsupported HTTP method. Only GET and POST are supported" +
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
        state_ = State::ERROR;
        spdlog::critical("Error in onRequestStart: {}", e.what());
    };
}

bool X402Processor::proxyResponseToBackEnd(std::string settlementInfo) {
    std::string backendResponseBody;
    std::string errorMessage;
    bool success = BackendConnection::proxyToBackEnd(backendResponseBody, errorMessage);
    if (!success) {
        reply502BadGateway(errorMessage.empty() ? "Failed to fetch content from upstream service." : errorMessage);
        return true;
    }
    reply200Success(settlementInfo, backendResponseBody);
    return false;
}

void X402Processor::onRequestCompletion(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    const string& body) noexcept {
    try {
        if (state_ == State::ERROR) return;

        CHECK_STATE(organization_);
        resource_ = organization_->getResourceByPath(decodedPath_, method_, body);

        if (!resource_) {
            reply400BadRequest("Resource not found for path: " + decodedPath_);
            return;
        }

        std::string settlementInfo;
        if (!hasValidPaymentHeader(reqHeaders, settlementInfo)) {
            reply402PaymentRequired();
            return;
        }
        state_ = State::PAYMENT_HEADER_RECEIVED;
        if (proxyResponseToBackEnd(settlementInfo)) return;
    } catch (std::exception &e) {
        state_ = State::ERROR;
        spdlog::critical("Error in onRequestStart: {}", e.what());
    }
}

void X402Processor::onBodySizeIncrease(size_t newSize) {
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024; // 128 KB
    spdlog::info("Request body size increased: {} bytes", newSize);
    if (newSize > MAX_BODY_SIZE) {
        reply400BadRequest("Request body too large. Maximum allowed is 1MByte. You can increase this limit in "
                           "machinepay config if needed.");
        state_ = State::ERROR;
    }
}
