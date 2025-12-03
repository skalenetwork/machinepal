#pragma once

#include "MachinePayCommon.h"
#include "x402_server/ServerFactory.h"
#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <proxygen/httpserver/HTTPServer.h>

class PaymentPayload;

struct HttpResponse {
    uint64_t status = 0;
    std::string body;
    proxygen::HTTPHeaders headers;
};

struct X402Client {
private:
    std::string baseUrl_;

public:
    X402Client(const std::string &baseUrl);

    ~X402Client();

    std::string baseUrl();

    HttpResponse
    sendGetRequestAndParseResult(std::string _location,
                              const std::vector<pair<string, string> > &_requestHeaders);

    HttpResponse sendRequestWithPayloadAndParseResult(
        std::string _location, ptr<PaymentPayload> payload);
};
