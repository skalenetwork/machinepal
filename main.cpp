#include <MachinePayCommon.h>
#include "src/x402_server/X402Handler.h"
#include "src/x402_server/ServerFactory.h"
#include "src/MachinePayApp.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/ssl/SSLContextConfig.h>
#include "CLI/CLI.hpp"
#include <folly/init/Init.h>
#include "config/ConfigManager.h"
#include "init/Init.h"

#include <map>
#include <regex>

#include "admin/ProjectGenerator.h"
#include "admin/ClientCli.h"


using namespace proxygen;


void setIfNotEmpty(std::map<std::string, std::string> &envOverloads, const std::string &key, const std::string &value);

void setIfNotEmpty(std::map<std::string, std::string> &envOverloads, const std::string &key, const std::string &value) {
    if (!value.empty()) {
        envOverloads[key] = value;
    }
}

void setBooleanSwitchIfNotEmpty(std::map<std::string, std::string> &envOverloads, const std::string &key, bool value) {
    if (value) {
        envOverloads[key] = "true";
    }
}


map<string, string> parseConfigValueOverloadsFromCommandLineAndEnvironment(int argc, char **argv) {
    try {
        // get environment overloads first. Then command line can override them.
        auto envOverloads = Init::getMachinePayEnvironmentOverloads();
        // Use CLI11 to parse command line
        std::string configFilePath;
        std::string logLevel;
        std::string logType;
        std::string bindIp;
        std::string hostname;
        CLI::App app{"machinepay"};
        app.add_option("-c,--config", configFilePath,
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
        app.add_option("--bind-ip", bindIp,
                       "Bind IP address for the server")
                ->type_name("IP");
        app.add_option("--hostname", hostname,
                       "Hostname for the server")
                ->type_name("HOSTNAME");

        // Client submenu options
        ClientConfig clientConfig;
        ClientCli::addClientSubcommand(app, clientConfig);

        app.add_subcommand("init",
                           "Initialize a new machinepay project in the current working directory");

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError &e) {
            auto code = app.exit(e);
            exit(code);
        }

        if (app.get_subcommand("client")->parsed()) {
            auto returnCode = ClientCli::runClientCommand(clientConfig);
            exit(returnCode);
        } else if (app.get_subcommand("init")->parsed()) {
            auto returnCode = ProjectGenerator::generateProjectInCurrentWorkingDir();
            exit(returnCode);
        }


        setIfNotEmpty(envOverloads, "CONFIG", configFilePath);
        setIfNotEmpty(envOverloads, "LOG_LEVEL", logLevel);
        setIfNotEmpty(envOverloads, "LOG_TYPE", logType);
        setIfNotEmpty(envOverloads, "BIND_IP", bindIp);
        setIfNotEmpty(envOverloads, "HOSTNAME", hostname);


        if (!envOverloads.contains("CONFIG")) {
            // If config file is not set, set to default ./machinepay.yml
            envOverloads["CONFIG"] = "./machinepay.yml";
        }

        return envOverloads;
    } catch
    (
        const std::exception &ex
    ) {
        spdlog::critical("Error parsing commmand line and environment", ex.what());
        printNestedException(ex);
        exit(1);
    } catch
    (
        ...
    ) {
        spdlog::critical("Unknown error loading config.");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    try {
        Init::initAllLibs(1, argv);
        auto configValueOverloads = parseConfigValueOverloadsFromCommandLineAndEnvironment(argc, argv);

        auto configFilePath = configValueOverloads.at("CONFIG");

        if (configFilePath == "./machinepay.yml") {
            spdlog::info("Using default config path ./machinepay.yml");
        } else {
            spdlog::info("Using config path: {}", configFilePath);
        }

        if (configValueOverloads.size() > 0) {
            spdlog::info("Values set in command line and environment override "
                "the corresponding configuration file values."
                " Command line takes precedence over environment.");
            for (const auto &kv: configValueOverloads) {
                spdlog::info("{} = {}", kv.first, kv.second);
            }
        }

        Init::checkOperatingSystemConfiguration();


        auto machinePayApp = MachinePayApp::makeInstance(configValueOverloads);
        machinePayApp->runUntilExit();
        return 0;
    } catch (const std::exception &ex) {
        spdlog::critical("Fatal error in main. Exiting. ");
        printNestedException(ex);
        return 1;
    } catch (...) {
        spdlog::critical("Unknown fatal error in main. Exiting.");
        return 1;
    }
}
