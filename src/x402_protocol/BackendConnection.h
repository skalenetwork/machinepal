#pragma once

#include <string>

namespace proxygen {
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static bool proxyToBackEndGet( std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndPost( const std::string& requestBody, std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndHead( std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndOptions( std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndPut( const std::string& requestBody, std::string& backendResponseBody, std::string& errorMessage );
public:

    static bool proxyToBackEnd( proxygen::HTTPMethod method_ ,
    const std::string& requestBody,
        std::string& backendResponseBody, std::string& errorMessage );

};
