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
    void reply402PaymentRequired(IResponseSender& downstream);
    void sendResponse(IResponseSender& downstream, const std::pair<uint16_t, std::string>& statusAndMessage,
                      const std::vector<std::pair<std::string, std::string>>& headers, const std::string& body);
    void reply400BadRequest(IResponseSender& downstream, const std::string& message);
    void reply502BadGateway(IResponseSender& downstream, const std::string& message);
    static bool isPathValid(const std::string& path, string& errorMessage);
    void reply200Success(IResponseSender& downstream, const std::string& settlementInfo,
                                std::string proxyBody);


    MachinePayApp& app_;
    bool responseSent_ = false;
};
