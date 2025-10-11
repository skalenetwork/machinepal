#include "common.h"
#include "X402Processor.h"
#include "MachinePayApp.h"
#include "IResponseSender.h"
#include <folly/json.h>
#include "BackendConnection.h"
#include "examples/PaymentExamples.h"

#include "boost/url/decode_view.hpp"
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>


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
        paymentInfo = R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"SDC\",\"network\":\"base-1net\"})";
        return true;
    }
    return false;
}



void X402Processor::reply200Success(IResponseSender& downstream, const std::string& settlementInfo, std::string proxyBody)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"},
        {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    downstream.sendResponse({200, "OK"}, headers, proxyBody);
}


void X402Processor::reply402PaymentRequired(IResponseSender& downstream) {
    folly::dynamic req = folly::dynamic::object;
    auto paymentRequirements = EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA;
    req = folly::parseJson(paymentRequirements);
    auto json = folly::toJson(req);
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/json"}
    };
    downstream.sendResponse({402, "Payment Required"}, headers, json);
}




void X402Processor::reply400BadRequest(IResponseSender& downstream, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    downstream.sendResponse({400, "Bad Request"}, headers, message);
}


void X402Processor::reply502BadGateway(IResponseSender& downstream, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    downstream.sendResponse({502, "Bad Gateway"}, headers, message);
}

bool X402Processor::isPathValid(const std::string& path, std::string& errorMessage)
{
    std::string decodedPath;
    try {
        auto decoded = boost::urls::decode_view(path);
        decodedPath = std::string(decoded.begin(), decoded.end());
    } catch (const std::exception& e) {
        errorMessage = "Path contains invalid characters";
        return false;
    }
    if (decodedPath.empty()) {
        errorMessage = "Empty path";
        return false;
    }
    if (decodedPath.front() != '/') {
        errorMessage = "Non-rooted path";
        return false;
    }
    // Reject traversal attempts (including encoded)
    if (decodedPath.find("..") != std::string::npos) {
        errorMessage = "Path traversal not allowed";
        return false;
    }


    std::wstring wide_text = boost::locale::conv::to_utf<wchar_t>(decodedPath,  "UTF-8");

    for (wchar_t ch : wide_text) {
        if (iswalnum(ch) || ch == L'/') {
            continue;
        }
        return false;
    }

    return true;
}

void X402Processor::onRequestStart(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders, IResponseSender& downstream)
{
    CHECK_STATE(reqHeaders);
    auto path = reqHeaders->getQueryString();
    std::string errorMessage;
    if (!isPathValid(path, errorMessage) ) {
        reply400BadRequest(downstream, errorMessage);
    }
}

void X402Processor::onRequestCompletion(IResponseSender& responseSender, const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders) {
    std::string settlementInfo;
    if (!hasValidPaymentHeader(reqHeaders, settlementInfo)) {
        reply402PaymentRequired(responseSender);
        return;
    }

    std::string backendResponseBody;
    std::string errorMessage;
    bool success = BackendConnection::proxyToBackEnd(backendResponseBody, errorMessage);
    if (!success) {
        reply502BadGateway(responseSender, errorMessage.empty() ? "Failed to fetch content from upstream service." : errorMessage);
        return;
    }
    reply200Success(responseSender, settlementInfo, backendResponseBody);
}
