#pragma once
#include "IResponseSender.h"



namespace proxygen {
    class ResponseHandler;
    class HTTPMessage;
}

class MachinePayApp; // Forward declaration

class X402Processor {
public:
    explicit X402Processor(MachinePayApp& app);

    void onRequestStart(const std::unique_ptr<proxygen::HTTPMessage>& headers, IResponseSender& downstream);
    void onRequestCompletion(IResponseSender& responseSender, const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders);
private:

    static bool hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>&  _req, std::string& paymentInfo);
    static void reply402PaymentRequired(IResponseSender& downstream);
    static void reply400BadRequest(IResponseSender& downstream, const std::string& message);
    static void reply502BadGateway(IResponseSender& downstream, const std::string& message);
    static bool isPathValid(const std::string& path, string& errorMessage);
    static void reply200Success(IResponseSender& downstream, const std::string& settlementInfo,
                                std::string proxyBody);


    MachinePayApp& app_;
};
