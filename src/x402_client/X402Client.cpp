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


HttpResponse X402Client::doGetRequest(
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


HttpResponse X402Client::doPostRequest(
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


HttpResponse X402Client::doX402GetRequest(
    std::string _location, ptr<PaymentPayload> payload) {
    std::vector<pair<string, string> > header;
    if ( payload) {
        header.push_back(payload->createHttpHeaderValue());
    }
    return doGetRequest(_location, {header});
}


HttpResponse X402Client::doX402PostRequest(
    const std::string& _location,
    ptr<PaymentPayload> payload,
    const std::string& requestBody) {
    std::vector<pair<string, string> > header;
    if ( payload) {
        header.push_back(payload->createHttpHeaderValue());
    }
    return doPostRequest(_location, {header}, requestBody);
}