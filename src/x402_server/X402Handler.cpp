#include "common.h"
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
        reqHeaders_ = std::move(_headers);
        std::shared_ptr<IResponseSender>  responseSender = std::make_shared<ProxygenResponseSender>(downstream_) ;
        processor_ = app_.makeX402Processor(responseSender);
        processor_->onRequestStart(reqHeaders_);
    } catch (const std::exception &ex)
    {
        spdlog::critical("X402Handler::onRequest() exception: {}", ex.what());
    } catch (...)
    {
        spdlog::critical("X402Handler::onRequest() unknown exception");
    }
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    try
    {
        if (!_body) return;
        _body->coalesce();
        bodyBuffer_.append(reinterpret_cast<const char *>(_body->data()), _body->length());
    } catch (const std::exception &ex)
    {
        spdlog::critical("X402Handler::onBody() exception: {}", ex.what());
    } catch (...)
    {
        spdlog::critical("X402Handler::onBody() unknown exception");
    }
}



void X402Handler::onEOM() noexcept {
    ProxygenResponseSender responseSender(downstream_);
    if (!processor_)
    {
        spdlog::critical("X402Handler::onEOM() called without processor_");
        return;
    }
    try
    {
        processor_->onRequestCompletion(reqHeaders_);
    } catch (const std::exception &ex)
    {
        spdlog::critical("X402Handler::onEOM() exception: {}", ex.what());
    } catch (...)
    {
        spdlog::critical("X402Handler::onEOM() unknown exception");
    }
}
