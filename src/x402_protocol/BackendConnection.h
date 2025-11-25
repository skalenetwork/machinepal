#pragma once

#include <string>

namespace proxygen {
    class HTTPMessage;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static bool proxyToBackEndGet(  const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndPost(  const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, const std::string& requestBody, std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndHead(  const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndOptions(  const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndPut(  const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, const std::string& requestBody, std::string& backendResponseBody, std::string& errorMessage );
public:

    static bool proxyToBackEnd( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders, proxygen::HTTPMethod method_ ,
    const std::string& requestBody,
        std::string& backendResponseBody, std::string& errorMessage );

};
