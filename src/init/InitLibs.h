#pragma once
#include <curl/curl.h>
#include <folly/init/Init.h>
#include <glog/logging.h>
#include <mutex>
#include <atomic>

class InitLibs {
public:
    static void initAll(int _argc, char* _argv[]);
    static bool isInited();
private:
    static std::atomic<bool> inited_;
};
