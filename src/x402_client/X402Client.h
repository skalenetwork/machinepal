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


public:
    X402Client();

    ~X402Client();

    std::string baseUrl();

    HttpResponse
    doGetRequest(std::string _location,
                              const std::vector<pair<string, string> > &_requestHeaders);

    HttpResponse doPostRequest(const std::string &_location,
                                               const std::vector<std::pair<std::string, std::string>> &_requestHeaders,
                                               const std::string &requestBody);

    HttpResponse doX402GetRequest(
        std::string _location, ptr<PaymentPayload> payload);

    HttpResponse doX402PostRequest(const std::string &_location, ptr<PaymentPayload> payload,
                                                          const std::string &requestBody);
};
