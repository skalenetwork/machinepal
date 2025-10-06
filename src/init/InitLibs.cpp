//
// Created by kladko on 9/29/25.
//

#include "InitLibs.h"

std::atomic<bool> InitLibs::inited_{false};

void InitLibs::initAll(int _argc, char* _argv[]) {
    if (!inited_.exchange(true)) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        FLAGS_logtostderr = 1;
        FLAGS_minloglevel = google::INFO;
        static folly::Init init(&_argc, &_argv);  // Static to preserve lifetime, pass by pointer
        LOG(INFO) << "Libraries initialized";
    }
}

bool InitLibs::isInited() {
    return inited_;
}
