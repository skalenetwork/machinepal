#pragma once
#include <curl/curl.h>
#include <folly/init/Init.h>
#include <glog/logging.h>
#include <mutex>

class InitLibs {
public:
    static void initAll(int _argc, char* _argv[]) {
        static std::once_flag init_flag;
        std::call_once(init_flag, [&]() {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            FLAGS_logtostderr = 1;
            FLAGS_minloglevel = google::GLOG_INFO;
            static folly::Init init(&_argc, &_argv);  // Static to preserve lifetime
        });
    }

};
