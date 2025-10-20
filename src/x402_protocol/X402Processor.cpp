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


X402Processor::X402Processor(MachinePayApp& app, ptr<IResponseSender>& responseSender)
    : app_(app), responseSender_(responseSender)
{
    // TODO: Add any initialization logic if needed
}

bool X402Processor::hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>& req, std::string& paymentInfo)
{
    try
    {
        const auto& headerTable = req->getHeaders();
        std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
        if (payment.empty()) return false;
        if (payment == "demo-ok")
        {
            paymentInfo =
                R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"SDC\",\"network\":\"base-1net\"})";
            return true;
        }
    }
    catch (std::exception& e)
    {
        spdlog::error("Error parsing payment header: {}", e.what());
    }
    return false;
}


void X402Processor::reply200Success(const std::string& settlementInfo,
                                    std::string proxyBody)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"},
        {"X-PAYMENT-RESPONSE", settlementInfo}
    };
    sendResponse({200, "OK"}, headers, proxyBody);
    state_ = State::SUCCESS;
}


std::string X402Processor::getPaymentRequirementsAsString() {
    auto paymentRequirements = EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA;
    return paymentRequirements;
}

void X402Processor::reply402PaymentRequired()
{

    folly::dynamic req = folly::dynamic::object;
    auto paymentRequirements = getPaymentRequirementsAsString();
    req = folly::parseJson(paymentRequirements);
    auto json = folly::toJson(req);
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/json"}
    };
    sendResponse({402, "Payment Required"}, headers, json);
    state_ = State::PAYMENT_REQUIRED_SENT;
}

void X402Processor::reply400BadRequest(const std::string& message)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    sendResponse({400, "Bad Request"}, headers, message);
    state_ = State::ERROR;
}


void X402Processor::reply502BadGateway(const std::string& message)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "text/plain"}
    };
    sendResponse({502, "Bad Gateway"}, headers, message);
    state_ = State::ERROR;
}


void X402Processor::sendResponse(
                                 const std::pair<uint16_t, std::string>& statusAndMessage,
                                 const std::vector<std::pair<std::string, std::string>>& headers,
                                 const std::string& body = "")
{
    CHECK_STATE(state_ != State::ERROR);
    CHECK_STATE(responseSender_);
    responseSender_->sendResponse(statusAndMessage, headers, body);
}


bool X402Processor::decodePath(const std::string& path, std::string& errorMessage)
{
    try
    {
        if (path.empty())
        {
            errorMessage = "Empty URL path " + path;
            goto error;
        }


        std::string decodedPath;
        try
        {
            auto decoded = boost::urls::decode_view(path);
            decodedPath = std::string(decoded.begin(), decoded.end());
        }
        catch (const std::exception& e)
        {
            errorMessage = "Path contains invalid characters";
            goto error;;
        }
        if (decodedPath.empty())
        {
            errorMessage = "Empty decoded URL path " + path;
            goto error;
        }
        if (decodedPath.front() != '/')
        {
            errorMessage = "URL path does not start with '/'";
            goto error;;;
        }
        // Reject traversal attempts (including encoded)
        if (decodedPath.find("..") != std::string::npos)
        {
            errorMessage = "URL path traversal not allowed";
            goto error;;
        }


        std::wstring wideText = boost::locale::conv::to_utf<wchar_t>(decodedPath, "UTF-8");

        for (wchar_t ch : wideText)
        {
            auto isValid = iswalnum(ch) || ch == L'/';
            if (!isValid)
            {
                errorMessage = "URL path contains invalid character:" + path;
                goto error;
            }
        }

        return true;
    }
    catch (std::exception& e)
    {
        errorMessage = e.what();
        goto error;
    }

error:
    spdlog::error("Error parsing user submitted URL path in X402Processor: {}", errorMessage);
    return false;
}

void X402Processor::onRequestStart(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders)
    noexcept
{
    try
    {
        CHECK_STATE(reqHeaders);
        auto path = reqHeaders->getPath();
        std::string errorMessage;
        if (!decodePath(path, errorMessage))
        {
            reply400BadRequest(errorMessage);
        }
    }
    catch (std::exception& e)
    {
        state_ = State::ERROR;
        spdlog::critical("Error in onRequestStart: {}", e.what());
    };
}

bool X402Processor::proxyResponseToBackEnd(std::string settlementInfo)
{
    std::string backendResponseBody;
    std::string errorMessage;
    bool success = BackendConnection::proxyToBackEnd(backendResponseBody, errorMessage);
    if (!success)
    {
        reply502BadGateway(errorMessage.empty() ? "Failed to fetch content from upstream service." : errorMessage);
        return true;
    }
    reply200Success(settlementInfo, backendResponseBody);
    return false;
}

void X402Processor::onRequestCompletion(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders) noexcept
{
    try
    {
        if (state_ == State::ERROR) return;
        std::string settlementInfo;
        if (!hasValidPaymentHeader(reqHeaders, settlementInfo))
        {
            reply402PaymentRequired();
            return;
        }
        state_ = State::PAYMENT_HEADER_RECEIVED;
        if (proxyResponseToBackEnd(settlementInfo)) return;
    }
    catch (std::exception& e)
    {
        state_ = State::ERROR;
        spdlog::critical("Error in onRequestStart: {}", e.what());
    }
}
