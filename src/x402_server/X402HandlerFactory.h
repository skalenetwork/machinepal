// X402HandlerFactory.h
#pragma once
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/RequestHandler.h>

#include "MachinePayApp.h"
#include "X402Handler.h"
#include "config/ConfigManager.h"

class MachinePayApp;

class X402HandlerFactory : public proxygen::RequestHandlerFactory {
public:
    explicit X402HandlerFactory(MachinePayApp &app)
        : app_(app) {
    }

    void onServerStart(folly::EventBase* /*_evb*/) noexcept override {}
    void onServerStop() noexcept override {}

    proxygen::RequestHandler* onRequest(proxygen::RequestHandler* /*_handler*/,
                                        proxygen::HTTPMessage* /*_msg*/) noexcept override {
        return new X402Handler(app_.configManager()->latestConfig());
    }

private:

    MachinePayApp & app_;
};
