#pragma once


class X402Processor;
class IResponseSender;

class BackendConnection {
public:
    static bool proxyToBackEnd( std::string& backendResponseBody, std::string& errorMessage );
    static bool proxyToBackEndPost( const std::string& requestBody, std::string& backendResponseBody, std::string& errorMessage );
};
