#pragma once
#include "IResponseSender.h"
#include "X402ProcessorState.h"



namespace proxygen {
    class ResponseHandler;
    class HTTPMessage;
}

class MachinePayApp; // Forward declaration

class X402Processor {
public:
    using State = x402::State;
    explicit X402Processor(MachinePayApp& app, ptr<IResponseSender>& responseSender);

    void onRequestStart(const std::unique_ptr<proxygen::HTTPMessage>& headers) noexcept;
    void onRequestCompletion(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders) noexcept;
private:

    bool hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>&  _req, std::string& paymentInfo);
    void reply402PaymentRequired();
    void sendResponse(const std::pair<uint16_t, std::string>& statusAndMessage,
                      const std::vector<std::pair<std::string, std::string>>& headers, const std::string& body);
    void reply400BadRequest( const std::string& message);
    void reply502BadGateway(const std::string& message);
    bool decodePath(const std::string& path, string& errorMessage);
    void reply200Success(const std::string& settlementInfo,
                                std::string proxyBody);


    MachinePayApp& app_;
    std::string path_;
    std::string decodedPath_;
    ptr<IResponseSender> responseSender_;
    State state_ = State::START;
};
