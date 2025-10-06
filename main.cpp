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


void checkExistsAndReadable(std::string configFile) {
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        throw std::runtime_error(
            "Config file '" + configFile + "' does not exist. Failed to get current working directory.");
    }

    // Check that configFile exists
    if (!std::filesystem::exists(configFile)) {
        throw std::runtime_error(
            "Config file '" + configFile + "' does not exist. Current working directory: " + std::string(cwd));
    }
    // Check that configFile is not a directory
    if (std::filesystem::is_directory(configFile)) {
        throw std::runtime_error(
            "Config file '" + configFile + "' is a directory, not a file. Current working directory: " +
            std::string(cwd));
    }
    // Check that configFile is readable
    std::ifstream configTest(configFile);
    if (!configTest.good()) {
        char cwd2[4096];
        if (!getcwd(cwd2, sizeof(cwd2))) {
            throw std::runtime_error(
                "Config file '" + configFile + "' is not readable. Failed to get current working directory.");
        }
        throw std::runtime_error(
            "Config file '" + configFile + "' is not readable. Current working directory: " + std::string(cwd2));
    }
    configTest.close();
}

int parseCommandLine(int argc, char **argv) {
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

        if(const char* envConfig = std::getenv("MACHINEPAY_CONFIG")) {
            configFile = envConfig;
        }


        checkExistsAndReadable(configFile);
        MachinePayConfigManager::loadConfig(configFile);
    } catch (const std::exception &ex) {
        std::cerr << "Error lading config: " << ex.what();
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error loading config";
        exit(1);
    }


    return 0;
}

int main(int argc, char *argv[]) {


    parseCommandLine(argc, argv);

    try {
        InitLibs::initAll(1, argv);


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
