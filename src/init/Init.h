#pragma once
#include <curl/curl.h>
#include <folly/init/Init.h>
#include <glog/logging.h>
#include <atomic>
#include <map>
#include <mutex>
#include <string>

class ConfigManager;

class Init {
public:
    static void initAllLibs( int _argc, char* _argv[] );
    static bool isInited();
    static std::map< std::string, std::string > getMachinePalEnvironmentOverloads();
    static void checkOperatingSystemConfiguration();

    static void initLogLevelFromConfig( ptr< ConfigManager > manager );
    static void checkSystemTime();

private:
    static std::atomic< bool > inited_;

    static bool fetchInternetTime( const char* url, std::string& utc_datetime,
        std::string& responseOut, std::string& errorOut );
};

void ThrowOnFailure();