#include <common.h>
#include "src/x402_server/X402Handler.h"
#include "src/x402_server/ServerFactory.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/ssl/SSLContextConfig.h>
#include "CLI/CLI.hpp"
#include <folly/init/Init.h>
#include "config/MachinePayConfigManager.h"
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

    try {
        // Use CLI11 to parse command line
        std::string configFile = "machinepay.yml";
        CLI::App app{"machinepay"};
        app.add_option("-c,--config", configFile, "Path to config file")->default_val("machinepay.yml");
        CLI11_PARSE(app, argc, argv);
        MachinePayConfigManager::loadConfig(configFile);
        auto serverConfig = MachinePayConfigManager::latestConfig()->server();
        auto serverObject = ServerFactory::createServerInstance(*serverConfig);
        serverObject->start();
    } catch (std::exception& ex) {
        LOG(ERROR) << "Fatal error thrown in main: ";
        printNestedException(ex);
        return 1;
    } catch (...) {
        LOG(ERROR) << "Unknown fatal error thrown in main";
        return 1;
    }
    return 0;
}
