#pragma once

#include <folly/json.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <string>

#include "MachinePayApp.h"
#include "config/MachinePayConfig.h"

class MachinePayApp;

class X402Handler : public proxygen::RequestHandler {
public:
    void onRequest( std::unique_ptr< proxygen::HTTPMessage > _headers ) noexcept override;
    void onBody( std::unique_ptr< folly::IOBuf > _body ) noexcept override;
    void sendInternalError();
    void onEOM() noexcept override;

    void requestComplete() noexcept override {
        // clean object if not used by different thread
        self_.reset();
    }

    ~X402Handler() override {}

    void onError( proxygen::ProxygenError _err ) noexcept override {
        spdlog::error( "X402Handler::onError called: {}", proxygen::getErrorString( ( _err ) ) );
        // clean object if not used by different thread
        self_.reset();
    }

    void onUpgrade( proxygen::UpgradeProtocol /*_prot*/ ) noexcept override {
        // No upgrade handling needed for now
    }

    explicit X402Handler( MachinePayApp& app ) : app_( app ) {
        // we take the latest condig at the start
        config_ = app_.configManager()->latestConfig();
        CHECK_STATE( config_ );
    }

private:
    MachinePayApp& app_;
    ptr< MachinePayConfig > config_;
    std::unique_ptr< proxygen::HTTPMessage > reqHeaders_;
    std::string bodyBuffer_;
    ptr< IProcessor > processor_;
    ptr< X402Handler > self_{ nullptr };

    friend class X402HandlerFactory;
};