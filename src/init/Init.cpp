//
// Created by kladko on 9/29/25.
//
#include "MachinePayCommon.h"

#include <glog/logging.h>
#include "Init.h"

#include "config/ConfigManager.h"
#include "config/subconfigs/LogConfig.h"


// New: header callback to collect raw headers
static size_t HeaderCallback( char* buffer, size_t size, size_t nitems, void* userdata ) {
    size_t total = size * nitems;
    auto* headers = static_cast< string* >( userdata );
    headers->append( buffer, total );
    return total;
}

atomic< bool > Init::inited_{ false };


void ThrowOnFailure() {
    cerr << "Fatal log or CHECK failed in proxygen" << endl;
    throw runtime_error( "Fatal log or CHECK failed" );
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
            throw runtime_error( "Duplicate environment variable: " + string( key ) );
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
    const char* url, string& utcDatetime, string& responseOut, string& errorOut ) {
    CURL* curl = curl_easy_init();
    if ( !curl ) {
        errorOut = "Failed to initialize curl for time check";
        return false;
    }

    // RAII for curl handle
    auto curl_cleanup = [&]() {
        if ( curl ) {
            curl_easy_cleanup( curl );
            curl = nullptr;
        }
    };
    shared_ptr< void > guard( nullptr, [&]( void* ) { curl_cleanup(); } );


    string headerBuffer;
    // First try: HEAD request to extract Date header
    curl_easy_setopt( curl, CURLOPT_URL, url );
    curl_easy_setopt( curl, CURLOPT_NOBODY, 1L );
    curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, HeaderCallback );
    curl_easy_setopt( curl, CURLOPT_HEADERDATA, &headerBuffer );
    curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5L );
    CURLcode res = curl_easy_perform( curl );

    auto parseDateHeader = [&]( const string& headers ) -> bool {
        istringstream iss( headers );
        string line;
        while ( getline( iss, line ) ) {
            if ( line.ends_with( "\r" ) )
                line.pop_back();
            // Case-insensitive starts_with "date:"
            if ( line.size() >= 5 ) {
                string prefix = line.substr( 0, 5 );
                for ( auto& c : prefix )
                    c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                if ( prefix == "date:" ) {
                    string value = line.substr( 5 );
                    // trim leading spaces
                    while (
                        !value.empty() && isspace( static_cast< unsigned char >( value.front() ) ) )
                        value.erase( value.begin() );
                    // Expected: Sun, 16 Nov 2025 12:19:42 GMT
                    // Remove trailing GMT if present for parsing
                    if ( value.size() > 4 && value.substr( value.size() - 4 ) == " GMT" ) {
                        value = value.substr( 0, value.size() - 4 );
                    }
                    tm tm{};
                    istringstream parse( value );
                    parse >> get_time( &tm, "%a, %d %b %Y %H:%M:%S" );
                    if ( !parse.fail() ) {
                        time_t t = timegm( &tm );
                        if ( t != -1 ) {
                            auto gmt = gmtime( &t );
                            if ( gmt ) {
                                ostringstream out;
                                out << put_time( gmt, "%Y-%m-%dT%H:%M:%SZ" );
                                utcDatetime = out.str();
                                return true;
                            }
                        }
                    }
                    return false;
                }
            }
        }
        return false;
    };

    if ( res == CURLE_OK && parseDateHeader( headerBuffer ) ) {
        responseOut = headerBuffer;
        return true;
    } else {
        return false;
    }
}

void Init::checkSystemTime() {
    string utcDatetime, response, error;
    bool ok = fetchInternetTime( "https://google.com", utcDatetime, response, error );
    if ( !ok ) {
        spdlog::warn( "fetchInternetTime failed: {}", error );
        return;
    }
    if ( utcDatetime.empty() ) {
        spdlog::warn( "Empty utcDatetime received. Response: {}", response );
        return;
    }

    // Expect formats like:
    // 1) YYYY-MM-DDTHH:MM:SSZ
    // 2) YYYY-MM-DDTHH:MM:SS.ffffffZ
    // 3) YYYY-MM-DDTHH:MM:SS+00:00
    // 4) YYYY-MM-DDTHH:MM:SS.ffffff+00:00
    // 5) YYYY-MM-DDTHH:MM:SS(.fraction)+00:00
    if ( utcDatetime.size() < 19 ) {
        spdlog::warn( "utcDatetime too short: {}", utcDatetime );
        spdlog::warn( "Full response: {}", response );
        return;
    }

    string base = utcDatetime.substr( 0, 19 );  // YYYY-MM-DDTHH:MM:SS
    tm tm{};
    istringstream ss( base );
    ss >> get_time( &tm, "%Y-%m-%dT%H:%M:%S" );
    if ( ss.fail() ) {
        spdlog::warn( "Failed to parse base datetime: {}", base );
        spdlog::warn( "Full utcDatetime: {}", utcDatetime );
        return;
    }

    // Parse remainder for fractional seconds and timezone
    int offsetSeconds = 0;
    size_t idx = 19;
    // Skip fractional seconds if present
    if ( idx < utcDatetime.size() && utcDatetime[idx] == '.' ) {
        ++idx;
        while ( idx < utcDatetime.size() &&
                isdigit( static_cast< unsigned char >( utcDatetime[idx] ) ) )
            ++idx;
    }

    if ( idx < utcDatetime.size() ) {
        char tzChar = utcDatetime[idx];
        if ( tzChar == 'Z' ) {
            // UTC, no offset
        } else if ( tzChar == '+' || tzChar == '-' ) {
            int sign = ( tzChar == '+' ) ? 1 : -1;
            ++idx;
            if ( idx + 4 < utcDatetime.size() ) {  // HH:MM (5 chars)
                string hhStr = utcDatetime.substr( idx, 2 );
                string mmStr = utcDatetime.substr( idx + 3, 2 );  // skip colon
                if ( utcDatetime[idx + 2] == ':' && isdigit( hhStr[0] ) && isdigit( hhStr[1] ) &&
                     isdigit( mmStr[0] ) && isdigit( mmStr[1] ) ) {
                    int hh = stoi( hhStr );
                    int mm = stoi( mmStr );
                    offsetSeconds = sign * ( hh * 3600 + mm * 60 );
                } else {
                    spdlog::warn( "Malformed timezone segment in utcDatetime: {}", utcDatetime );
                    return;
                }
            } else {
                spdlog::warn( "Incomplete timezone segment in utcDatetime: {}", utcDatetime );
                return;
            }
        } else {
            spdlog::warn(
                "Unexpected character after datetime '{}' in '{}'", utcDatetime[idx], utcDatetime );
            return;
        }
    }

    time_t baseUtc = timegm( &tm );
    if ( baseUtc == -1 ) {
        spdlog::warn( "timegm failed for '{}'", base );
        return;
    }
    // If offset is +HH:MM, local time ahead of UTC, so UTC = local - offset.
    time_t internetTime = baseUtc - offsetSeconds;
    time_t systemTime = time( nullptr );
    long diff = labs( systemTime - internetTime );

    spdlog::info(
        "System time: {} | Internet time: {} | Diff: {} seconds", systemTime, internetTime, diff );
    if ( diff > 60 ) {
        throw runtime_error(
            "System time differs from internet time by more than 60 seconds. Synchronize system "
            "time before starting machinepay." );
    }
}

void Init::checkOperatingSystemConfiguration() {
    utsname buffer{};
    if ( uname( &buffer ) != 0 ) {
        throw runtime_error( "Failed to get OS information" );
    }
    if ( string( buffer.sysname ) != "Linux" ) {
        throw runtime_error(
            "Unsupported OS: " + string( buffer.sysname ) + ". Only Linux is supported." );
    }
    spdlog::info( "Operating system: {}", buffer.sysname );

    checkSystemTime();
}