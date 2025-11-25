//
// Created by stan on 11/10/25.
//
#include "BackendConnection.h"
#include "MachinePayCommon.h"
#include "X402Processor.h"
#include "curl/curl.h"


bool BackendConnection::proxyToBackEnd(proxygen::HTTPMethod method_,
    const std::string& requestBody,std::string &backendResponseBody,
    std::string &errorMessage) {
    switch (method_) {
        case proxygen::HTTPMethod::GET:
            return proxyToBackEndGet(backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::POST:
            // For POST, we need to pass an empty body as we don't have it here
            return proxyToBackEndPost(requestBody, backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::HEAD:
            return proxyToBackEndHead(backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::OPTIONS:
            return proxyToBackEndOptions(backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::PUT:
            return proxyToBackEndPut(requestBody, backendResponseBody, errorMessage);
        default:
            errorMessage = "Unsupported HTTP method for backend proxying.";
            return false;
    }
}

bool BackendConnection::proxyToBackEndGet(
    std::string& backendResponseBody, std::string& errorMessage ) {
    static thread_local std::unique_ptr< CURL, decltype( &curl_easy_cleanup ) > curlThreadLocal(
        nullptr, &curl_easy_cleanup );

    if ( !curlThreadLocal ) {
        auto curlObject = curl_easy_init();
        if ( !curlObject ) {
            spdlog::error( "Could not initialize CURL object" );
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset( curlObject );
    }
    // Fetch content from the external URL
    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl );

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
            auto* str = static_cast< std::string* >( _userdata );
            str->append( _ptr, _size * _nmemb );
            return _size * _nmemb;
        } );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &backendResponseBody );

    auto result = curl_easy_perform( curl );

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
    }

    curl_easy_cleanup( curl );

    if ( result != CURLE_OK ) {
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    } else {
        return true;
    }
}

bool BackendConnection::proxyToBackEndPost(
    const std::string& requestBody,
    std::string& backendResponseBody,
    std::string& errorMessage ) {
    static thread_local std::unique_ptr< CURL, decltype( &curl_easy_cleanup ) > curlThreadLocal(
        nullptr, &curl_easy_cleanup );

    if ( !curlThreadLocal ) {
        auto curlObject = curl_easy_init();
        if ( !curlObject ) {
            spdlog::error( "Could not initialize CURL object" );
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset( curlObject );
    }

    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl );

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts" );

    // Set POST with body
    curl_easy_setopt( curl, CURLOPT_POST, 1L );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, requestBody.c_str() );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, static_cast<long>( requestBody.size() ) );

    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
            auto* str = static_cast< std::string* >( _userdata );
            str->append( _ptr, _size * _nmemb );
            return _size * _nmemb;
        } );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &backendResponseBody );

    auto result = curl_easy_perform( curl );

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
    }

    curl_easy_cleanup( curl );

    if ( result != CURLE_OK ) {
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    } else {
        return true;
    }
}

bool BackendConnection::proxyToBackEndHead(std::string &backendResponseBody,
    std::string &errorMessage) {
    static thread_local std::unique_ptr< CURL, decltype( &curl_easy_cleanup ) > curlThreadLocal(
        nullptr, &curl_easy_cleanup );

    if ( !curlThreadLocal ) {
        auto curlObject = curl_easy_init();
        if ( !curlObject ) {
            spdlog::error( "Could not initialize CURL object" );
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset( curlObject );
    }

    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl );

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    // HEAD request to the same sample endpoint as GET
    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

    // Configure HEAD: no body
    curl_easy_setopt( curl, CURLOPT_NOBODY, 1L );

    // Capture response headers (if needed by caller)
    curl_easy_setopt(
        curl, CURLOPT_HEADERFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
            auto* str = static_cast< std::string* >( _userdata );
            str->append( _ptr, _size * _nmemb );
            return _size * _nmemb;
        } );
    curl_easy_setopt( curl, CURLOPT_HEADERDATA, &backendResponseBody );

    auto result = curl_easy_perform( curl );

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
    }

    curl_easy_cleanup( curl );

    if ( result != CURLE_OK ) {
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    } else {
        return true;
    }
}

bool BackendConnection::proxyToBackEndOptions(std::string &backendResponseBody,
    std::string &errorMessage) {
    static thread_local std::unique_ptr< CURL, decltype( &curl_easy_cleanup ) > curlThreadLocal(
        nullptr, &curl_easy_cleanup );

    if ( !curlThreadLocal ) {
        auto curlObject = curl_easy_init();
        if ( !curlObject ) {
            spdlog::error( "Could not initialize CURL object" );
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset( curlObject );
    }

    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl );

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    // Use same sample endpoint as GET/HEAD
    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

    // Configure OPTIONS request
    curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, "OPTIONS" );

    // OPTIONS response usually has no body; capture headers
    curl_easy_setopt( curl, CURLOPT_NOBODY, 1L );
    curl_easy_setopt(
        curl, CURLOPT_HEADERFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
            auto* str = static_cast< std::string* >( _userdata );
            str->append( _ptr, _size * _nmemb );
            return _size * _nmemb;
        } );
    curl_easy_setopt( curl, CURLOPT_HEADERDATA, &backendResponseBody );

    auto result = curl_easy_perform( curl );

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
    }

    curl_easy_cleanup( curl );

    if ( result != CURLE_OK ) {
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    } else {
        return true;
    }
}

bool BackendConnection::proxyToBackEndPut(const std::string &requestBody, std::string &backendResponseBody,
    std::string &errorMessage) {
    static thread_local std::unique_ptr< CURL, decltype( &curl_easy_cleanup ) > curlThreadLocal(
        nullptr, &curl_easy_cleanup );

    if ( !curlThreadLocal ) {
        auto curlObject = curl_easy_init();
        if ( !curlObject ) {
            spdlog::error( "Could not initialize CURL object" );
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset( curlObject );
    }

    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl );

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    // Use a sample endpoint; for PUT we target a specific resource
    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

    // Configure PUT with body
    curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, "PUT" );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, requestBody.c_str() );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, static_cast<long>( requestBody.size() ) );

    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
            auto* str = static_cast< std::string* >( _userdata );
            str->append( _ptr, _size * _nmemb );
            return _size * _nmemb;
        } );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &backendResponseBody );

    auto result = curl_easy_perform( curl );

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
    }

    curl_easy_cleanup( curl );

    if ( result != CURLE_OK ) {
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    } else {
        return true;
    }
}
