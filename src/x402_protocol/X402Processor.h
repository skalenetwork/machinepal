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
    bool hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>&  _req, std::string& paymentInfo);
    void reply402(IResponseSender& downstream);
    void reply400(IResponseSender& downstream, const std::string& message);
    void reply502(IResponseSender& downstream, const std::string& message);
    bool processUrlAndHeaders(const std::unique_ptr<proxygen::HTTPMessage>& headers, IResponseSender& downstream);
    void reply200(IResponseSender& downstream, const std::string& settlementInfo,
                 std::string proxyBody);
private:
    MachinePayApp& app_;
};
