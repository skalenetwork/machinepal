#include "X402Client.h"

#include "payment/datastructures/PaymentPayload.h"
#include <functional>
#include "x402_protocol/HttpEndpointConnection.h"
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "x402_protocol/IBackendError.h"

X402Client::X402Client(const std::string &baseUrl)
    : baseUrl_(baseUrl) {
}

X402Client::~X402Client() {
}

std::string X402Client::baseUrl() {
    return baseUrl_;
}


HttpResponse X402Client::sendGetRequestAndParseResult(
    std::string _location, const std::vector<pair<string, string> > &_requestHeaders) {
    std::string url = baseUrl() + _location;

    auto requestHeaders = proxygen::HTTPHeaders();

    for (const auto &header: _requestHeaders) {
        requestHeaders.add(header.first, header.second);
    }


    HttpResponse resp;


    HttpEndpointConnection httpEndpointConnection(url, true);

    auto err = httpEndpointConnection.doGetRequest(requestHeaders,
                                                   resp.status, resp.headers, resp.body);


    return {resp};
}


HttpResponse X402Client::sendPostRequestAndParseResult(
    const std::string& _location,
    const std::vector<std::pair<std::string, std::string>>& _requestHeaders,
    const std::string& requestBody) {
    std::string url = baseUrl() + _location;

    auto requestHeaders = proxygen::HTTPHeaders();

    for (const auto& header : _requestHeaders) {
        requestHeaders.add(header.first, header.second);
    }

    HttpResponse resp;

    HttpEndpointConnection httpEndpointConnection(url, true);

    auto err = httpEndpointConnection.doPostRequest(requestHeaders, requestBody,
                                                    resp.status, resp.headers, resp.body);

    return resp;
}


HttpResponse X402Client::sendGetRequestWithPayloadAndParseResult(
    std::string _location, ptr<PaymentPayload> payload) {
    CHECK_STATE(payload);
    auto header = payload->createHttpHeaderValue();
    return sendGetRequestAndParseResult(_location, {header});
}


HttpResponse X402Client::sendPostRequestWithPayloadAndParseResult(
    const std::string& _location,
    ptr<PaymentPayload> payload,
    const std::string& requestBody) {
    CHECK_STATE(payload);
    auto header = payload->createHttpHeaderValue();
    return sendPostRequestAndParseResult(_location, {header}, requestBody);
}