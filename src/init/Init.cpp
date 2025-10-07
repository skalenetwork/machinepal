//
// Created by kladko on 9/29/25.
//
#include "common.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include "Init.h"

#include <regex>

std::atomic<bool> Init::inited_{false};

void Init::initAllLibs(int _argc, char* _argv[]) {
    if (!inited_.exchange(true)) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        FLAGS_logtostderr = 1;
        FLAGS_minloglevel = google::INFO;
        static folly::Init init(&_argc, &_argv);  // Static to preserve lifetime, pass by pointer


        auto logger = spdlog::stderr_logger_mt("machinepay");
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);  // Set global log level to INFO
       // spdlog::set_pattern(
         //   R"({"ts":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","logger":"%n","pid":%P,"tid":%t,"msg":"%v"})");
        spdlog::info("Libraries initialized");
    }
}

bool Init::isInited() {
    return inited_;
}


std::map<std::string, std::string> Init::getAllMachinePayEnvVars() {
    std::map<std::string, std::string> envVars;
    extern char **environ;
    std::regex re("^MACHINE_PAY_");
    for (char **env = environ; *env != nullptr; ++env) {
        std::string entry(*env);
        auto pos = entry.find('=');
        if (pos != std::string::npos) {
            std::string key = entry.substr(0, pos);
            std::string value = entry.substr(pos + 1);
            if (std::regex_search(key, re)) {
                envVars[key] = value;
            }
        }
    }
    return envVars;
}