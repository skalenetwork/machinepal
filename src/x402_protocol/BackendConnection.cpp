//
// Created by stan on 11/10/25.
//
#include "BackendConnection.h"

#include <proxygen/lib/http/HTTPMessage.h>
#include "MachinePayCommon.h"
#include "X402Processor.h"
#include "curl/curl.h"
#include <spdlog/spdlog.h>
#include <algorithm> // Required for string trimming




bool BackendConnection::proxyToBackEnd(
                                       proxygen::HTTPMethod method_,
                                       const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                       const std::string &requestBody, vector<pair<string, string>> &responseHeaders,
                                       std::string &responseBody,
                                       std::string &errorMessage) {
    switch (method_) {
        case proxygen::HTTPMethod::GET:
            return proxyToBackEndGet(requestHeaders, responseHeaders, responseBody, errorMessage);
        case proxygen::HTTPMethod::POST:
            return proxyToBackEndPost(requestHeaders, requestBody, responseHeaders, responseBody, errorMessage);
        case proxygen::HTTPMethod::HEAD:
            return proxyToBackEndHead(requestHeaders, responseHeaders, errorMessage);
        case proxygen::HTTPMethod::OPTIONS:
            return proxyToBackEndOptions(requestHeaders, responseHeaders, responseBody, errorMessage);
        case proxygen::HTTPMethod::PUT:
            return proxyToBackEndPut(requestHeaders, requestBody, responseHeaders, responseBody, errorMessage);
        default:
            errorMessage = "Unsupported HTTP method for backend proxying.";
            return false;
    }
}

bool BackendConnection::proxyToBackEndGet(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                          vector<pair<string, string>> &responseHeaders,
                                          std::string &responseBody, std::string &errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype( &curl_easy_cleanup )> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    // --- Process Request Headers ---
    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");

    // --- Setup Response Header Parsing ---
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // --- Setup Response Body Parsing ---
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
        auto* str = static_cast< std::string* >( _userdata );
        str->append( _ptr, _size * _nmemb );
        return _size * _nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndPost(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                           const std::string &requestBody,
                                           vector<pair<string, string>> &responseHeaders,
                                           std::string &responseBody,
                                           std::string &errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype( &curl_easy_cleanup )> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts");

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>( requestBody.size() ));

    // --- Setup Response Header Parsing ---
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // --- Setup Response Body Parsing ---
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
        auto* str = static_cast< std::string* >( _userdata );
        str->append( _ptr, _size * _nmemb );
        return _size * _nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndHead(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                           vector<pair<string, string>> &responseHeaders,
                                           std::string &errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype( &curl_easy_cleanup )> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");

    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

    // --- Setup Response Header Parsing ---
    // In previous code, HEADERFUNCTION wrote to responseBody.
    // For HEAD, we want to populate responseHeaders.
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndOptions(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                              vector<pair<string, string>> &responseHeaders,
                                              std::string &responseBody,
                                              std::string &errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype( &curl_easy_cleanup )> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");

    // 1. Set the method
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");

    // 2. REMOVED: curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    // OPTIONS responses might contain a body (e.g., WADL documentation or detailed specs).
    // We must allow curl to read it, otherwise the stream might misalign or we lose data.

    // 3. Setup Body Parsing (Required if server sends body)
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
        auto* str = static_cast< std::string* >( _userdata );
        str->append( _ptr, _size * _nmemb );
        return _size * _nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    // 4. Setup Response Header Parsing
    // Assumes 'headerCallback' is defined as in the previous step
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

bool BackendConnection::proxyToBackEndPut(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                          const std::string &requestBody, vector<pair<string, string>> &responseHeaders,
                                          std::string &responseBody,
                                          std::string &errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype( &curl_easy_cleanup )> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage = "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>( requestBody.size() ));

    // --- Setup Response Header Parsing ---
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // --- Setup Response Body Parsing ---
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[]( char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) -> size_t {
        auto* str = static_cast< std::string* >( _userdata );
        str->append( _ptr, _size * _nmemb );
        return _size * _nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        errorMessage = "Failed to fetch content from upstream service.";
        return false;
    }

    return true;
}

struct curl_slist * BackendConnection::createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders) {
    struct curl_slist *chunk = nullptr;
    if (requestHeaders) {
        requestHeaders->getHeaders().forEach([&chunk](const std::string &name, const std::string &value) {
            std::string headerStr = name + ": " + value;
            chunk = curl_slist_append(chunk, headerStr.c_str());
        });
    }
    return chunk;
}


std::string BackendConnection::trimWhiteSpaceFromHeader(const std::string& str) {
    const char* whitespace = " \t\r\n";
    size_t first = str.find_first_not_of(whitespace);
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(whitespace);
    return str.substr(first, (last - first + 1));
}


size_t BackendConnection::headerCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t totalSize = size * nitems;
    std::string raw(buffer, totalSize);

    // FIX: Cast to vector<pair<string, string>>*, not map*
    auto* headers = static_cast<std::vector<std::pair<std::string, std::string>>*>(userdata);

    // HTTP headers come in the format "Key: Value"
    size_t colonPos = raw.find(':');
    if (colonPos != std::string::npos) {
        std::string key = trimWhiteSpaceFromHeader(raw.substr(0, colonPos));
        std::string value = trimWhiteSpaceFromHeader(raw.substr(colonPos + 1));

        if (!key.empty()) {
            // FIX: Use emplace_back for vector
            headers->emplace_back(key, value);
        }
    }
    return totalSize;
}