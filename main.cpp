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




std::string parseCommandLineAndEnvironmentOverloads(int argc, char **argv) {
    try {
        // Use CLI11 to parse command line

        std::string configFilePathFromCli;
        // Check for environment variable override

        CLI::App app{"machinepay"};
        app.add_option("-c,--config", configFilePathFromCli,
            "Path to the config file. Default is ./machinepay.yml. "
            "Can be overwritten by MACHINEPAY_CONFIG environment variable.")
            ->default_val(configFilePathFromCli)
            ->type_name("FILE");
        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError &e) {
            auto code = app.exit(e);
            exit(code);
        }


        auto envOverloads = Init::getMachinePayEnvironmentOverloads();

        string configFilePath = "machinepay.yml";

        if (!configFilePathFromCli.empty()) {
            configFilePath = configFilePathFromCli;
        } else {
            if (envOverloads.contains("CONFIG")) {
                configFilePath = envOverloads["CONFIG"];
            }
        }

        auto it = envOverloads.find("CONFIG");
        if (it != envOverloads.end()) {
            envOverloads.erase(it);
        }

        return configFilePathFromCli;

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

        auto configFile = parseCommandLineAndEnvironmentOverloads(argc, argv);

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
