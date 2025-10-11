#include "common.h"
#include "X402Handler.h"
#include <folly/json.h>
#include <curl/curl.h>

#include "ProxygenResponseSender.h"
#include "examples/PaymentExamples.h"
#include "x402_protocol/BackendConnection.h"

using namespace proxygen;


void X402Handler::onRequest(std::unique_ptr<HTTPMessage> _headers) noexcept {
    reqHeaders_ = std::move(_headers);
    ProxygenResponseSender responseSender(downstream_) ;
    app_.x402Processor()->onRequestStart(reqHeaders_, responseSender);
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    if (!_body) return;
    _body->coalesce();
    bodyBuffer_.append(reinterpret_cast<const char *>(_body->data()), _body->length());
}



void X402Handler::onEOM() noexcept {
    ProxygenResponseSender responseSender(downstream_);
    app_.x402Processor()->onRequestCompletion(responseSender, reqHeaders_);
}
