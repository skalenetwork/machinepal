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

std::string X402Client::parseStatusLineAndHeaders( const std::vector< std::string >& _headersVector,
    std::map< std::string, std::string >& _headersMap ) {
    std::string statusLine;
    if ( !_headersVector.empty() ) {
        statusLine = _headersVector[0];
        statusLine.erase( statusLine.find_last_not_of( " \t\r\n" ) + 1 );
    }
    for ( size_t _i = 1; _i < _headersVector.size(); ++_i ) {
        auto _pos = _headersVector[_i].find( ':' );
        if ( _pos != std::string::npos ) {
            std::string _key = _headersVector[_i].substr( 0, _pos );
            std::string _value = _headersVector[_i].substr( _pos + 1 );
            _key.erase( 0, _key.find_first_not_of( " \t\r\n" ) );
            _key.erase( _key.find_last_not_of( " \t\r\n" ) + 1 );
            _value.erase( 0, _value.find_first_not_of( " \t\r\n" ) );
            _value.erase( _value.find_last_not_of( " \t\r\n" ) + 1 );
            _headersMap[_key] = _value;
        }
    }
    return statusLine;
}

HttpResponse X402Client::sendRequestAndParseResult(
    std::string _location, const std::vector<pair<string, string> >& _extraHeaders, bool printHttpTrace ) {
    (void)printHttpTrace; // currently unused with proxyToBackEnd public API
    // Build the full URL
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
    auto err = BackendConnection::proxyToBackEnd(
        url, proxygen::HTTPMethod::GET, requestHeaders, std::string{},
        httpStatusCode, responseHeaders, responseBody, printHttpTrace);
    (void)err; // We continue to return the response details regardless of error ptr

    // Build HttpResponse compatible with previous usage
    HttpResponse resp;
    resp.status = static_cast<long>(httpStatusCode);
    resp.body = std::move(responseBody);
    resp.headers = responseHeaders;



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


