#include "common.h"
#include "X402Processor.h"
#include "MachinePayApp.h"
#include "IResponseSender.h"
#include <folly/json.h>

#include "BackendConnection.h"
#include "examples/PaymentExamples.h"




X402Processor::X402Processor(MachinePayApp& app)
    : app_(app)
{
    // TODO: Add any initialization logic if needed
}

bool X402Processor::hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>& req, std::string& paymentInfo) {
    // TODO: Parse and verify real X-PAYMENT header.
    // This stub accepts "demo-ok".
    const auto& headerTable = req->getHeaders();
    std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
    if (payment.empty()) return false;
    if (payment == "demo-ok") {
        paymentInfo = R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"USDC\",\"network\":\"base-1net\"})";
        return true;
    }
    return false;
}



void X402Processor::reply200(IResponseSender& downstream, const std::string& settlementInfo, std::string proxyBody)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"},
        {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    downstream.sendResponse({200, "OK"}, headers, proxyBody);
}


void X402Processor::reply402(IResponseSender& downstream) {
    folly::dynamic req = folly::dynamic::object;
    auto paymentRequirements = EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA;
    req = folly::parseJson(paymentRequirements);
    auto json = folly::toJson(req);
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/json"}
    };
    downstream.sendResponse({402, "Payment Required"}, headers, json);
}




void X402Processor::reply400(IResponseSender& downstream, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    downstream.sendResponse({400, "Bad Request"}, headers, message);
}


void X402Processor::reply502(IResponseSender& downstream, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    downstream.sendResponse({502, "Bad Gateway"}, headers, message);
}

bool X402Processor::processUrlAndHeaders(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders, IResponseSender& downstream) {
    CHECK_STATE(reqHeaders);
    auto path = reqHeaders->getPath();
    // Reject empty or non-rooted paths
    if (path.empty() || path.front() != '/') {
        reply400(downstream, "Invalid or insecure path");
        return true;
    }
    // Allow only [A-Za-z0-9] and '/'
    bool badChar = std::any_of(path.begin(), path.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '/');
    });
    if (badChar) {
        reply400(downstream, "Path contains invalid characters");
        return true;
    }
    // Optionally: reject traversal attempts
    if (path.find("..") != std::string::npos) {
        reply400(downstream, "Path traversal not allowed");
        return true;
    }
    return false;
}

void X402Processor::doOnEOM(IResponseSender& responseSender, const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders) {
    std::string settlementInfo;
    if (hasValidPaymentHeader(reqHeaders, settlementInfo)) {
        auto errorString = BackendConnection::proxyToBackEnd(this, responseSender, settlementInfo);
        if (!errorString.empty()) {
            reply502(responseSender, "Failed to fetch content from upstream service.");
        }
    } else {
        reply402(responseSender);
    }
}
