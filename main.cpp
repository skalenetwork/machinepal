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
        return new X402Handler(MachinePayConfigManager::getInstance().latestConfig());
    }
};




map<string, string>  parseCommandLineAndEnvironmentOverloads(int argc, char **argv) {
    try {

        // get environment overloads first. Then command line can override them.
        auto envOverloads = Init::getMachinePayEnvironmentOverloads();
        // Use CLI11 to parse command line
        std::string configFilePathFromCli;
        std::string logLevel;
        std::string logType;
        CLI::App app{"machinepay"};
        app.add_option("-c,--config", configFilePathFromCli,
            "Path to the config file. Default is ./machinepay.yml.")
            ->type_name("FILE");
        app.add_option("-l,--log-level", logLevel,
            "Log level: trace, debug, info, warn, error, fatal")
            ->type_name("LOG_LEVEL")
            ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error", "fatal"}));
        app.add_option("-t,--log-type", logType,
            "Log type: plain, json")
            ->type_name("LOG_TYPE")
            ->check(CLI::IsMember({"plain", "json"}));
        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError &e) {
            auto code = app.exit(e);
            exit(code);
        }

        if (!configFilePathFromCli.empty()) {
            envOverloads["CONFIG"] = configFilePathFromCli;
        };

        if (!logLevel.empty()) {
            envOverloads["LOG_LEVEL"] = logLevel;
        }

        if (!logType.empty()) {
            envOverloads["LOG_TYPE"] = logType;
        }

        if (envOverloads.size() > 0) {
            spdlog::info("Values set in command line and environment override "
                         "the corresponding configuration file values."
                         " Command line takes precedence over environment.");
            for (const auto& kv : envOverloads) {
                spdlog::info("{} = {}", kv.first, kv.second);
            }
        }
        return envOverloads;

    } catch (const std::exception &ex) {
        spdlog::critical("Error parsing commmand line and environment", ex.what());
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
        auto configValuesFromCliAndEnv = parseCommandLineAndEnvironmentOverloads(argc, argv);
        MachinePayConfigManager::getInstance().initManager(configValuesFromCliAndEnv);
        auto logConfig = MachinePayConfigManager::getInstance().latestConfig()->log();
        auto serverConfig = MachinePayConfigManager::getInstance().latestConfig()->server();
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
