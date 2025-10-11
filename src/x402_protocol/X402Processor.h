#pragma once


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
    static void reply402(proxygen::ResponseHandler* downstream);
    static void reply400(proxygen::ResponseHandler* downstream, const std::string& message);
    static void reply502(proxygen::ResponseHandler* downstream, const std::string& message);
    static bool processUrlAndHeaders(const proxygen::HTTPMessage* reqHeaders, proxygen::ResponseHandler* downstream);
    static void reply200(proxygen::ResponseHandler* downstream, const std::string& settlementInfo,
                         std::string proxyBody);
    static void proxyToBackEnd(proxygen::ResponseHandler* downstream, const std::string& settlementInfo);
private:
    MachinePayApp& app_;
};
