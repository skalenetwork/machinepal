#pragma once
#include "ResponseSender.h"


namespace proxygen
{
    class ResponseHandler;
    class HTTPMessage;
}

class MachinePayApp; // Forward declaration

class X402Processor {
public:
    explicit X402Processor(MachinePayApp& app);
    static bool hasValidPaymentHeader(const proxygen::HTTPMessage* _req, std::string& _paymentInfo);
    static void reply402(ResponseSender downstream);
    static void reply400(ResponseSender downstream, const std::string& message);
    static void reply502(ResponseSender downstream, const std::string& message);
    static bool processUrlAndHeaders(const proxygen::HTTPMessage* reqHeaders, ResponseSender downstream);
    static void reply200(ResponseSender downstream, const std::string& settlementInfo,
                         std::string proxyBody);
    static void proxyToBackEnd(ResponseSender downstream, const std::string& settlementInfo);
private:
    MachinePayApp& app_;
};
