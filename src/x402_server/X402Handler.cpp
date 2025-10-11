#include "common.h"
#include "X402Handler.h"
#include <folly/json.h>
#include <curl/curl.h>
#include "examples/PaymentExamples.h"

using namespace proxygen;


void X402Handler::onRequest(std::unique_ptr<HTTPMessage> _headers) noexcept {
    reqHeaders_ = std::move(_headers);
    X402Processor::processUrlAndHeaders(reqHeaders_.get(), ResponseSender(downstream_));
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    if (!_body) return;
    _body->coalesce();
    bodyBuffer_.append(reinterpret_cast<const char *>(_body->data()), _body->length());
}



void X402Handler::onEOM() noexcept {
    std::string settlementInfo;
    if (X402Processor::hasValidPaymentHeader(reqHeaders_.get(), settlementInfo)) {
        X402Processor::proxyToBackEnd(ResponseSender(downstream_), settlementInfo);
    } else {
        X402Processor::reply402(ResponseSender(downstream_));
    }
}
