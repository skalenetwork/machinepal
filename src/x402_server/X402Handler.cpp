#include "MachinePayCommon.h"
#include "X402Handler.h"
#include <folly/json.h>
#include <curl/curl.h>

#include "ProxygenResponseSender.h"
#include "examples/PaymentExamples.h"
#include "x402_protocol/BackendConnection.h"

using namespace proxygen;


void X402Handler::onRequest(std::unique_ptr<HTTPMessage> _headers) noexcept {
    try
    {
        CHECK_STATE(self_);
        reqHeaders_ = std::move(_headers);
        std::shared_ptr<IResponseSender>  responseSender = std::make_shared<ProxygenResponseSender>(downstream_) ;
        processor_ = app_.makeX402Processor(responseSender);
        processor_->onRequestStart(reqHeaders_);
    } catch (const std::exception& e)
    {
        spdlog::critical("Error in onRequest: {}", e.what());
    }
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    try
    {
        CHECK_STATE(self_);
        if (!_body) return;
        _body->coalesce();
        bodyBuffer_.append(reinterpret_cast<const char *>(_body->data()), _body->length());
        processor_->onBodySizeIncrease(bodyBuffer_.size());
    } catch (const std::exception& e)
    {
        spdlog::critical("Error in onBody: {}", e.what());
    }
}



void X402Handler::onEOM() noexcept {
    try
    {
        CHECK_STATE(self_);
        ProxygenResponseSender responseSender(downstream_);
        if (!processor_)
        {
            spdlog::critical("X402Handler::onEOM() called without processor_");
            return;
        }
        processor_->onRequestCompletion(reqHeaders_, bodyBuffer_);
    } catch (const std::exception& e)
    {
        spdlog::critical("Error in onEOM: {}", e.what());
    }
}
