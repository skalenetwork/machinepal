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
#include "init/Init.h"

#include <map>
#include <regex>


using namespace proxygen;

class X402HandlerFactory : public RequestHandlerFactory {
public:
    void onServerStart(folly::EventBase *) noexcept override {
    }

    void onServerStop() noexcept override {
    }

    RequestHandler *onRequest(RequestHandler *, HTTPMessage *msg) noexcept override {
        // Route if needed (e.g., only gate /paid). Here we gate everything.
        (void) msg;
        return new X402Handler();
    }
};




std::string parseCommandLine(int argc, char **argv) {
    try {
        // Use CLI11 to parse command line
        std::string configFile = "machinepay.yml";
        // Check for environment variable override

        CLI::App app{"machinepay"};
        app.add_option("-c,--config", configFile,
            "Path to the config file. Default is ./machinepay.yml. "
            "Can be overwritten by MACHINEPAY_CONFIG environment variable.")
            ->default_val(configFile)
            ->type_name("FILE");
        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError &e) {
            auto code = app.exit(e);
            exit(code);
        }


        auto envVars = Init::getAllMachinePayEnvVars();

        if (envVars.contains("MACHINEPAY_CONFIG")) {
            configFile = envVars["MACHINEPAY_CONFIG"];
        }

        return configFile;

    } catch (const std::exception &ex) {
        spdlog::critical("Error loading config: {}", ex.what());
        printNestedException(ex);
        exit(1);
    } catch (...) {
        spdlog::critical("Unknown error loading config.");
        exit(1);
    }
}



int main(int argc, char *argv[]) {


    try {
        Init::initAllLibs(1, argv);

        auto configFile = parseCommandLine(argc, argv);

        MachinePayConfigManager::loadConfig(configFile);
        auto serverConfig = MachinePayConfigManager::latestConfig()->server();
        auto serverObject = ServerFactory::createServerInstance(*serverConfig);
        serverObject->start();
    } catch (std::exception &ex) {
        LOG(ERROR) << "Fatal error thrown in main: ";
        printNestedException(ex);
        return 1;
    } catch (...) {
        LOG(ERROR) << "Unknown fatal error thrown in main";
        return 1;
    }
    return 0;
}
