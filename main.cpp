#include "src/X402Handler.h"
#include "src/ServerFactory.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/ssl/SSLContextConfig.h>
#include <folly/init/Init.h>

#include "init/InitLibs.h"


using namespace proxygen;

class X402HandlerFactory : public RequestHandlerFactory {
public:
    void onServerStart(folly::EventBase*) noexcept override {}
    void onServerStop() noexcept override {}

    RequestHandler* onRequest(RequestHandler*, HTTPMessage* msg) noexcept override {
        // Route if needed (e.g., only gate /paid). Here we gate everything.
        (void)msg;
        return new X402Handler();
    }
};


int main(int argc, char* argv[]) {
    InitLibs::initAll(argc, argv);
    std::string bindIP = "0.0.0.0";
    uint64_t bindPort = 8080;
    auto serverObject = ServerFactory::createServerInstance(bindIP, bindPort);
    serverObject->start();
    return 0;
}
