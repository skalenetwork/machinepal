#pragma once


class X402Processor;
class IResponseSender;

class BackendConnection
{


public:
    static void proxyToBackEnd(X402Processor* processor, IResponseSender& downstream, const std::string& settlementInfo);

};
