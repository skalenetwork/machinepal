//
// Created by kladko on 9/29/25.
//
#include "common.h"


#include <glog/logging.h>
#include <stdexcept>
#include <execinfo.h>
#include <iostream>
#include <stdexcept>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include "Init.h"
#include <boost/algorithm/string/predicate.hpp>

#include "config/ConfigManager.h"


atomic<bool> Init::inited_{false};


void ThrowOnFailure() {
    std::cerr << "Fatal log or CHECK failed in proxygen" << std::endl;
    throw std::runtime_error("Fatal log or CHECK failed");
}

void Init::initAllLibs(int _argc, char *_argv[]) {
    if (!inited_.exchange(true)) {

        auto rc = curl_global_init(CURL_GLOBAL_DEFAULT);
        CHECK_STATE2(rc == CURLE_OK, "curl_global_init failed");

        FLAGS_logtostderr = 1;
        FLAGS_minloglevel = google::INFO;
        static folly::Init init(&_argc, &_argv); // Static to preserve lifetime, pass by pointer


        google::InstallFailureFunction(&ThrowOnFailure);

        auto logger = spdlog::stderr_logger_mt("machinepay");
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info); // Set global log level to INFO
        // spdlog::set_pattern(
        //   R"({"ts":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","logger":"%n","pid":%P,"tid":%t,"msg":"%v"})");
        spdlog::info("Libraries initialized");
    }
}

bool Init::isInited() {
    return inited_;
}


map<string, string> Init::getMachinePayEnvironmentOverloads() {
    map<string, string> envOverloads;
    extern char **environ;
    const string prefix = "MACHINE_PAY_";
    for (char **env = environ; *env != nullptr; ++env) {
        string environmentVariable(*env);
        if (!environmentVariable.starts_with(prefix))
            continue;
        auto pos = environmentVariable.find('=');
        if (pos == string::npos)
            continue;
        string key = environmentVariable.substr(0, pos);
        string strippedKey = key.substr(prefix.size());

        if (strippedKey.empty()) {
            continue;
        }

        if (envOverloads.contains(strippedKey) > 0) {
            throw std::runtime_error("Duplicate environment variable: " + string(key));
        }
        envOverloads[strippedKey] = environmentVariable.substr(pos + 1);
    }
    return envOverloads;
}



void Init::initLogLevelFromConfig(ptr<ConfigManager> manager) {
    CHECK_STATE(manager);
    auto logConfig = manager->latestConfig()->log();
    auto logLevel = logConfig->level();

    spdlog::level::level_enum spdlogLevel = spdlog::level::info;

    if (logLevel == LogLevel::trace) spdlogLevel = spdlog::level::trace;
    else if (logLevel == LogLevel::debug) spdlogLevel = spdlog::level::debug;
    else if (logLevel == LogLevel::info) spdlogLevel = spdlog::level::info;
    else if (logLevel == LogLevel::warn) spdlogLevel = spdlog::level::warn;
    else if (logLevel == LogLevel::error) spdlogLevel = spdlog::level::err;
    else if (logLevel == LogLevel::fatal) spdlogLevel = spdlog::level::critical;
    else
    {
        CHECK_STATE(false); // should never happen
    }

    spdlog::set_level(spdlogLevel);
}