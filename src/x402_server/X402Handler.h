#pragma once

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <folly/json.h>
#include <string>

#include "MachinePayApp.h"
#include "config/MachinePayConfig.h"

class MachinePayApp;

class X402Handler : public proxygen::RequestHandler {
public:
    void onRequest(std::unique_ptr<proxygen::HTTPMessage> _headers) noexcept override;
    void onBody(std::unique_ptr<folly::IOBuf> _body) noexcept override;
    void onEOM() noexcept override;
    void requestComplete() noexcept override { delete this; }
    void onError(proxygen::ProxygenError /*_err*/) noexcept override { delete this; }

    void onUpgrade(proxygen::UpgradeProtocol /*_prot*/) noexcept override {
        // No upgrade handling needed for now
    }

    explicit X402Handler(MachinePayApp& app)
        : app_(app)
    {
        // we take the latest condig at the start
        config_ = app_.configManager()->latestConfig();
        CHECK_STATE(config_);
    }

private:
    MachinePayApp& app_;
    ptr<MachinePayConfig> config_;

    static bool hasValidPaymentHeader(const proxygen::HTTPMessage* _req, std::string& _paymentInfo);
    void reply402();

    void proxyToBackEnd(std::string _settlementInfo);

    std::unique_ptr<proxygen::HTTPMessage> reqHeaders;
    std::string reqURL;
    std::string bodyBuffer;
};
