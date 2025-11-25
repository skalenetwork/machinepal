//
// Created by stan on 11/10/25.
//
#include "BackendConnection.h"

#include <proxygen/lib/http/HTTPMessage.h>

#include "MachinePayCommon.h"
#include "X402Processor.h"
#include "curl/curl.h"
#include <spdlog/spdlog.h>

// Helper to convert Proxygen headers to Curl linked list
static struct curl_slist* createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders) {
    struct curl_slist* chunk = nullptr;
    if (reqHeaders) {
        reqHeaders->getHeaders().forEach([&chunk](const std::string& name, const std::string& value) {
            std::string headerStr = name + ": " + value;
            chunk = curl_slist_append(chunk, headerStr.c_str());
        });
    }
    return chunk;
}

bool BackendConnection::proxyToBackEnd(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    proxygen::HTTPMethod method_,
    const std::string& requestBody,std::string &backendResponseBody,
    std::string &errorMessage) {
    switch (method_) {
        case proxygen::HTTPMethod::GET:
            return proxyToBackEndGet(reqHeaders, backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::POST:
            return proxyToBackEndPost(reqHeaders, requestBody, backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::HEAD:
            return proxyToBackEndHead(reqHeaders, backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::OPTIONS:
            return proxyToBackEndOptions(reqHeaders, backendResponseBody, errorMessage);
        case proxygen::HTTPMethod::PUT:
            return proxyToBackEndPut(reqHeaders, requestBody, backendResponseBody, errorMessage);
        default:
            errorMessage = "Unsupported HTTP method for backend proxying.";
            return false;
    }
}

bool BackendConnection::proxyToBackEndGet( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
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

    CHECK_STATE( curlThreadLocal );
    auto* curl = curlThreadLocal.get();
    curl_easy_reset( curl ); // Important: reset options from previous reuse

    // --- Process Headers ---
    struct curl_slist* headers = createCurlHeadersFromProxygen(reqHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // -----------------------

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

    // Clean up headers immediately after request
    if (headers) {
        curl_slist_free_all(headers);
    }

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndPost( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
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

    // --- Process Headers ---
    struct curl_slist* headers = createCurlHeadersFromProxygen(reqHeaders);
    // Important: For POST, we usually need Content-Type. If it's not in reqHeaders,
    // you might need to manually append it here, otherwise it relies on the caller.
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // -----------------------

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

    if (headers) {
        curl_slist_free_all(headers);
    }

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndHead( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    std::string &backendResponseBody,
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

    // --- Process Headers ---
    struct curl_slist* headers = createCurlHeadersFromProxygen(reqHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // -----------------------

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

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

    if (headers) {
        curl_slist_free_all(headers);
    }

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndOptions( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    std::string &backendResponseBody,
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

    // --- Process Headers ---
    struct curl_slist* headers = createCurlHeadersFromProxygen(reqHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // -----------------------

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

    curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, "OPTIONS" );
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

    if (headers) {
        curl_slist_free_all(headers);
    }

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndPut( const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
    const std::string &requestBody, std::string &backendResponseBody,
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

    // --- Process Headers ---
    struct curl_slist* headers = createCurlHeadersFromProxygen(reqHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // -----------------------

    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    curl_easy_setopt( curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1" );

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

    if (headers) {
        curl_slist_free_all(headers);
    }

    if ( result != CURLE_OK ) {
        spdlog::error( "CURL error: {}", curl_easy_strerror( result ) );
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}