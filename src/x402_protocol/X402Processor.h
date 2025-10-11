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
private:
    MachinePayApp& app_;
};
