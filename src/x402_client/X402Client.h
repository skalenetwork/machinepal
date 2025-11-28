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
    std::vector< pair<string, string> > headers;
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
    HttpResponse
    sendRequestAndParseResult( std::string _location,
        const std::vector< pair<string, string> >& _extraHeaders, bool printHttpTrace = false );

     HttpResponse sendRequestWithPayloadAndParseResult(
        std::string _location, ptr< PaymentPayload > payload, bool printHttpTrace );


};