//
// Created by kladko on 9/29/25.
//
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include "InitLibs.h"

std::atomic<bool> InitLibs::inited_{false};

void InitLibs::initAll(int _argc, char* _argv[]) {
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

bool InitLibs::isInited() {
    return inited_;
}
