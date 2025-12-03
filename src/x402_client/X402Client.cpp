#include "X402Client.h"

#include "payment/datastructures/PaymentPayload.h"
#include <functional>
#include "x402_protocol/HttpEndpointConnection.h"
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

X402Client::X402Client(const std::string &baseUrl)
    : baseUrl_(baseUrl) {
}

X402Client::~X402Client() {
}

std::string X402Client::baseUrl() {
    return baseUrl_;
}


HttpResponse X402Client::sendRequestAndParseResult(
    std::string _location, const std::vector<pair<string, string> > &_requestHeaders) {
    std::string url = baseUrl() + _location;

    auto requestHeaders = proxygen::HTTPHeaders();

    for (const auto &header: _requestHeaders) {
        requestHeaders.add(header.first, header.second);
    }


    HttpResponse resp;
    auto err = HttpEndpointConnection::doGetRequest(
        url, requestHeaders,
        resp.status, resp.headers, resp.body);


    return {resp};
}


HttpResponse X402Client::sendRequestWithPayloadAndParseResult(
    std::string _location, ptr<PaymentPayload> payload) {
    CHECK_STATE(payload);
    auto header = payload->createHttpHeaderValue();
    return sendRequestAndParseResult(_location, {header});
}
