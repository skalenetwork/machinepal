#include "X402Client.h"

#include "payment/datastructures/PaymentPayload.h"
#include <functional>
#include "x402_protocol/BackendConnection.h"
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

X402Client::X402Client( const std::string& _connect_ip, uint16_t _port )
    : connectHost( _connect_ip ), port( _port ) {}

X402Client::~X402Client() {}

std::string X402Client::baseUrl() {
    return "http://" + connectHost;
}


HttpResponse X402Client::sendRequestAndParseResult(
    std::string _location, const std::vector<pair<string, string> >& _extraHeaders, bool printHttpTrace ) {
    (void)printHttpTrace; // currently unused with proxyToBackEnd public API

    std::string url = baseUrl() + ":" + std::to_string(port) + _location;

    // Prepare request headers from "Key: Value" strings
    auto requestHeaders = std::make_unique<proxygen::HTTPMessage>();
    for (const auto &header : _extraHeaders) {
            requestHeaders->getHeaders().add(header.first, header.second);
    }

    uint64_t httpStatusCode = 0;
    std::vector<std::pair<std::string,std::string>> responseHeaders;
    std::string responseBody;

    // Execute via BackendConnection with GET method (public API)
    auto err = BackendConnection::proxyToBackEndGet(
        url, requestHeaders,
        httpStatusCode, responseHeaders, responseBody, printHttpTrace);

    // Build HttpResponse compatible with previous usage
    HttpResponse resp;
    resp.status = static_cast<long>(httpStatusCode);
    resp.body = std::move(responseBody);
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


