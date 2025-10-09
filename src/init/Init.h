#pragma once
#include <curl/curl.h>
#include <folly/init/Init.h>
#include <glog/logging.h>
#include <mutex>
#include <atomic>

class ConfigManager;

class Init {
public:
    static void initAllLibs(int _argc, char* _argv[]);
    static bool isInited();
    static std::map<std::string, std::string> getMachinePayEnvironmentOverloads();


    static void initLogLevelFromConfig(ptr<ConfigManager> manager);

private:
    static std::atomic<bool> inited_;
};
