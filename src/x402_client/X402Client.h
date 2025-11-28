#pragma once

#include "MachinePayCommon.h"
#include "x402_server/ServerFactory.h"
#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <proxygen/httpserver/HTTPServer.h>

class PaymentPayload;

struct HttpResponse {
    long status = 0;
    std::string body;
    std::vector< std::string > headers;
};

struct X402Client {
private:
    std::string connectHost;
    uint16_t port;

public:
    X402Client( const std::string& _connectIp, uint16_t _port );
    ~X402Client();

    std::string baseUrl();
    static std::string parseStatusLineAndHeaders( const std::vector< std::string >& _headersVector,
        std::map< std::string, std::string >& _headersMap );
    std::tuple< std::map< std::string, std::string >, std::string, HttpResponse >
    sendRequestAndParseResult( std::string _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    std::tuple< std::map< std::string, std::string >, std::string, HttpResponse >
    sendRequestWithPayloadAndParseResult(
        std::string _location, ptr< PaymentPayload > payload, bool printHttpTrace );

    static size_t writeBody( char* _ptr, size_t _size, size_t _nmemb, void* _userdata );
    static size_t writeHeader( char* _buffer, size_t _size, size_t _nitems, void* _userdata );
    static int debugCallback(
        CURL* handle, curl_infotype type, char* data, size_t size, void* userptr );

    HttpResponse httpGet( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    HttpResponse httpHead( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    HttpResponse httpOptions( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    HttpResponse httpPut( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    HttpResponse httpPost( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );

    HttpResponse httpDelete( const std::string& _baseURL, const std::string& _location,
        const std::vector< std::string >& _extraHeaders, bool printHttpTrace = false );
};