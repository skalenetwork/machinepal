//
// Created by kladko on 9/29/25.
//
#include "MachinePayCommon.h"


#include <execinfo.h>
#include <glog/logging.h>
#include <iostream>
#include <stdexcept>

#include "Init.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <sys/utsname.h>
#include <boost/algorithm/string/predicate.hpp>

#include "config/ConfigManager.h"
#include "config/subconfigs/LogConfig.h"

#include <curl/curl.h>
#include <ctime>
#include <iomanip>
#include <sstream>

// Helper function for libcurl write callback
static size_t WriteCallback( void* contents, size_t size, size_t nmemb, void* userp ) {
    ( ( std::string* ) userp )->append( ( char* ) contents, size * nmemb );
    return size * nmemb;
}

atomic< bool > Init::inited_{ false };


void ThrowOnFailure() {
    std::cerr << "Fatal log or CHECK failed in proxygen" << std::endl;
    throw std::runtime_error( "Fatal log or CHECK failed" );
}

void Init::initAllLibs( int _argc, char* _argv[] ) {
    if ( !inited_.exchange( true ) ) {
        auto rc = curl_global_init( CURL_GLOBAL_DEFAULT );
        CHECK_STATE2( rc == CURLE_OK, "curl_global_init failed" );

        FLAGS_logtostderr = 1;
        FLAGS_minloglevel = google::INFO;
        static folly::Init init( &_argc, &_argv );  // Static to preserve lifetime, pass by pointer


        google::InstallFailureFunction( &ThrowOnFailure );

        auto logger = spdlog::stderr_logger_mt( "machinepay" );
        spdlog::set_default_logger( logger );
        spdlog::set_level( spdlog::level::info );  // Set global log level to INFO
        // spdlog::set_pattern(
        //   R"({"ts":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","logger":"%n","pid":%P,"tid":%t,"msg":"%v"})");
        spdlog::info( "Libraries initialized" );
    }
}

bool Init::isInited() {
    return inited_;
}


map< string, string > Init::getMachinePayEnvironmentOverloads() {
    map< string, string > envOverloads;
    extern char** environ;
    const string prefix = "MACHINE_PAY_";
    for ( char** env = environ; *env != nullptr; ++env ) {
        string environmentVariable( *env );
        if ( !environmentVariable.starts_with( prefix ) )
            continue;
        auto pos = environmentVariable.find( '=' );
        if ( pos == string::npos )
            continue;
        string key = environmentVariable.substr( 0, pos );
        string strippedKey = key.substr( prefix.size() );

        if ( strippedKey.empty() ) {
            continue;
        }

        if ( envOverloads.contains( strippedKey ) > 0 ) {
            throw std::runtime_error( "Duplicate environment variable: " + string( key ) );
        }
        envOverloads[strippedKey] = environmentVariable.substr( pos + 1 );
    }
    return envOverloads;
}


void Init::initLogLevelFromConfig( ptr< ConfigManager > manager ) {
    CHECK_STATE( manager );
    auto logConfig = manager->latestConfig()->log();
    auto logLevel = logConfig->level();

    spdlog::level::level_enum spdlogLevel = spdlog::level::info;

    if ( logLevel == LogLevel::trace )
        spdlogLevel = spdlog::level::trace;
    else if ( logLevel == LogLevel::debug )
        spdlogLevel = spdlog::level::debug;
    else if ( logLevel == LogLevel::info )
        spdlogLevel = spdlog::level::info;
    else if ( logLevel == LogLevel::warn )
        spdlogLevel = spdlog::level::warn;
    else if ( logLevel == LogLevel::error )
        spdlogLevel = spdlog::level::err;
    else if ( logLevel == LogLevel::fatal )
        spdlogLevel = spdlog::level::critical;
    else {
        CHECK_STATE( false );  // should never happen
    }

    spdlog::set_level( spdlogLevel );
}

bool Init::fetchInternetTime(
    const char* url, std::string& utcDatetime, std::string& responseOut, std::string& errorOut ) {
    CURL* curl = curl_easy_init();
    if ( !curl ) {
        errorOut = "Failed to initialize curl for time check";
        return false;
    }
    std::string readBuffer;
    curl_easy_setopt( curl, CURLOPT_URL, url );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
    curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5L );
    CURLcode res = curl_easy_perform( curl );
    curl_easy_cleanup( curl );
    if ( res != CURLE_OK ) {
        errorOut = std::string( "Failed to fetch internet time from " ) + url + ": " +
                   curl_easy_strerror( res );
        return false;
    }
    responseOut = readBuffer;
    // Try to find utc_datetime (worldtimeapi.org)
    auto pos = readBuffer.find( "\"utc_datetime\":" );
    if ( pos != std::string::npos ) {
        pos = readBuffer.find( '"', pos + 15 );
        if ( pos != std::string::npos ) {
            auto end = readBuffer.find( '"', pos + 1 );
            if ( end != std::string::npos ) {
                utcDatetime = readBuffer.substr( pos + 1, end - pos - 1 );
                return true;
            }
        }
    }
    // Try to find dateTime (timeapi.io)
    pos = readBuffer.find( "\"dateTime\":" );
    if ( pos != std::string::npos ) {
        pos = readBuffer.find( '"', pos + 10 );
        if ( pos != std::string::npos ) {
            auto end = readBuffer.find( '"', pos + 1 );
            if ( end != std::string::npos ) {
                utcDatetime = readBuffer.substr( pos + 1, end - pos - 1 );
                return true;
            }
        }
    }
    errorOut = std::string( "Could not find UTC datetime in response from " ) + url;
    return false;
}

void Init::checkSystemTime() {
    std::string utcDatetime, response, error;
    bool ok = fetchInternetTime(
        "http://worldtimeapi.org/api/timezone/Etc/UTC", utcDatetime, response, error );
    if ( !ok ) {
        spdlog::warn( "{}", error );
        // Try fallback
        ok = fetchInternetTime(
            "https://timeapi.io/api/Time/current/zone?timeZone=UTC", utcDatetime, response, error );
        if ( !ok ) {
            spdlog::warn( "{}", error );
            return;
        }
    }
    // Example: "2025-10-23T12:34:56.123456+00:00" or "2025-10-23T12:34:56"
    // Parse the datetime string (ignore fractional seconds and timezone)
    std::string datetime = utcDatetime.substr( 0, 19 );
    std::tm tm = {};
    std::istringstream ss( datetime );
    ss >> std::get_time( &tm, "%Y-%m-%dT%H:%M:%S" );
    if ( ss.fail() ) {
        spdlog::warn( "Failed to parse utcDatetime: {}", utcDatetime );
        spdlog::warn( "Full response: {}", response );
        return;
    }
    time_t internetTime = timegm( &tm );
    time_t systemTime = time( nullptr );
    long diff = std::labs( systemTime - internetTime );
    spdlog::info(
        "System time: {} | Internet time: {} | Diff: {} seconds", systemTime, internetTime, diff );
    if ( diff > 60 ) {
        throw std::runtime_error(
            "System time differs from internet time by more than 60 seconds. Its too much for "
            "machinepay to operate correctly. Please synchronize system time and then start "
            "machinepay." );
    }
}

void Init::checkOperatingSystemConfiguration() {
    utsname buffer{};
    if ( uname( &buffer ) != 0 ) {
        throw std::runtime_error( "Failed to get OS information" );
    }
    if ( std::string( buffer.sysname ) != "Linux" ) {
        throw std::runtime_error(
            "Unsupported OS: " + std::string( buffer.sysname ) + ". Only Linux is supported." );
    }
    spdlog::info( "Operating system: {}", buffer.sysname );

    checkSystemTime();
}