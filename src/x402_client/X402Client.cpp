#include "X402Client.h"

#include "payment/datastructures/PaymentPayload.h"
#include <functional>
#include "x402_protocol/HttpEndpointConnection.h"
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

X402Client::X402Client( const std::string& _connect_ip, uint16_t _port )
    : connectHost( _connect_ip ), port( _port ) {}

X402Client::~X402Client() {}

std::string X402Client::baseUrl() {
    return "http://" + connectHost;
}


HttpResponse X402Client::sendRequestAndParseResult(
    std::string _location, const std::vector<pair<string, string> >& _requestHeaders, bool printHttpTrace ) {

    std::string url = baseUrl() + ":" + std::to_string(port) + _location;

    // Prepare request headers from "Key: Value" strings
    auto requestHeaders = std::make_unique<proxygen::HTTPMessage>();

    for (const auto &header : _requestHeaders) {
            requestHeaders->getHeaders().add(header.first, header.second);
    }

    vector<pair<string,string>> responseHeaders;

    HttpResponse resp;
    auto err = HttpEndpointConnection::doGetRequest(
        url, requestHeaders,
        resp.status, responseHeaders, resp.body, printHttpTrace);

    resp.headers.insert(responseHeaders.begin(), responseHeaders.end());

    for (const auto &kv : responseHeaders) {
        spdlog::info("{}: {}", kv.first, kv.second);
    }
    spdlog::info("BODY::{}", resp.body);

    return { resp };
}


HttpResponse  X402Client::sendRequestWithPayloadAndParseResult(
    std::string _location, ptr< PaymentPayload > payload, bool printHttpTrace ) {
    CHECK_STATE( payload );
    auto header = payload->createHttpHeaderValue();
    return sendRequestAndParseResult( _location, { header }, printHttpTrace );
}


