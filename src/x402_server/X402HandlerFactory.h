// X402HandlerFactory.h
#pragma once
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/RequestHandler.h>
#include "X402Handler.h"

class X402HandlerFactory : public proxygen::RequestHandlerFactory {
public:
    void onServerStart(folly::EventBase* /*_evb*/) noexcept override {}
    void onServerStop() noexcept override {}

    proxygen::RequestHandler* onRequest(proxygen::RequestHandler* /*_handler*/,
                                        proxygen::HTTPMessage* /*_msg*/) noexcept override {
        return new X402Handler();
    }
};