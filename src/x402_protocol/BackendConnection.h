#pragma once


class X402Processor;
class IResponseSender;

class BackendConnection
{


public:
    static string proxyToBackEnd(std::string& backendResponseBody);

};
