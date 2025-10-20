#pragma once
#include <proxygen/lib/http/HTTPMethod.h>

#include "IResponseSender.h"
#include "X402ProcessorState.h"


class ResourceConfig;

namespace proxygen {
    enum class HTTPMethod;
    class ResponseHandler;
    class HTTPMessage;
}

class MachinePayApp; // Forward declaration
class MachinePayConfig;
class OrganizationConfig;

class X402Processor {
public:
    using State = x402::State;
    explicit X402Processor(MachinePayApp& app, ptr<IResponseSender>& responseSender);

    void onRequestStart(const std::unique_ptr<proxygen::HTTPMessage>& headers) noexcept;
    bool proxyResponseToBackEnd(std::string settlementInfo);
    void onRequestCompletion(const std::unique_ptr<proxygen::HTTPMessage>& reqHeaders,
        const string& body) noexcept;
    void onBodySizeIncrease(size_t newSize);
private:

    bool hasValidPaymentHeader(const std::unique_ptr<proxygen::HTTPMessage>&  _req, std::string& paymentInfo);
    void reply402PaymentRequired();
    void sendResponse(const std::pair<uint16_t, std::string>& statusAndMessage,
                      const std::vector<std::pair<std::string, std::string>>& headers, const std::string& body);
    void reply400BadRequest( const std::string& message);
    void reply502BadGateway(const std::string& message);

    bool validateAndExtractSubDomainName(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders);

    bool validateAndDecodePath(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders);

    bool matchOrganization();

    bool validateMethod(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders);

    void reply200Success(const std::string& settlementInfo,
                         std::string proxyBody);

    std::string getPaymentRequirementsAsString();


    MachinePayApp& app_;
    ptr<MachinePayConfig> config_;
    //std::string path_;
    std::string decodedPath_;
    std::string subDomainName_;
    ptr<OrganizationConfig> organization_;
    ptr<ResourceConfig> resource_;
    ptr<IResponseSender> responseSender_;
    State state_ = State::START;
    // initially set to non-supported value
    proxygen::HTTPMethod method_ = proxygen::HTTPMethod::TRACE;



    // Getter for config_
    const std::shared_ptr<MachinePayConfig>& config() const {
        CHECK_STATE(config_);
        return config_;
    }
};
