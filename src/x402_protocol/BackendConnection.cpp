//
// Created by stan on 11/10/25.
//
#include "BackendConnection.h"

#include <proxygen/lib/http/HTTPMessage.h>
#include "MachinePayCommon.h"
#include "X402Processor.h"
#include "curl/curl.h"
#include <spdlog/spdlog.h>
#include <folly/String.h> // Required for folly::toLowerAscii and folly::trimWhitespace

#include "BackendCurlError.h"
#include "IBackendError.h"
#include "BackendHttpError.h"

// "https://jsonplaceholder.typicode.com/posts"


ptr<IBackendError> BackendConnection::proxyToBackEnd(const string &url,
                                                     proxygen::HTTPMethod method_,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     const std::string &requestBody,
                                                     uint64_t &httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody) {
    switch (method_) {
        case proxygen::HTTPMethod::GET:
            return proxyToBackEndGet(url, requestHeaders, httpStatusCode, responseHeaders, responseBody);
        case proxygen::HTTPMethod::POST:
            return proxyToBackEndPost(url, requestHeaders, requestBody, httpStatusCode, responseHeaders, responseBody);
        case proxygen::HTTPMethod::HEAD:
            return proxyToBackEndHead(url, requestHeaders, httpStatusCode, responseHeaders);
        case proxygen::HTTPMethod::OPTIONS:
            return proxyToBackEndOptions(url, requestHeaders, httpStatusCode, responseHeaders, responseBody);
        case proxygen::HTTPMethod::PUT:
            return proxyToBackEndPut(url, requestHeaders, requestBody, httpStatusCode, responseHeaders, responseBody);
        case proxygen::HTTPMethod::DELETE:
            return proxyToBackEndDelete(url, requestHeaders, httpStatusCode, responseHeaders, responseBody);
        default:
            return make_shared<BackendHttpError>(501);
    }
}

ptr<IBackendError> BackendConnection::executeCurlRequest(const string &url,
                                                         const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                         uint64_t &httpStatusCode,
                                                         vector<pair<string, string> > &responseHeaders,
                                                         std::string &responseBody,
                                                         const std::function<void(CURL *)> &configureMethod,
                                                         bool printHttpTrace) {
    static thread_local std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlThreadLocal(
        nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            throw runtime_error("Could not initialize CURL object");
        }
        curlThreadLocal.reset(curlObject);
    }

    CHECK_STATE(curlThreadLocal);
    auto *curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    struct curl_slist *headers = createCurlHeadersFromProxygen(requestHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Enable SSL certificate verification for security
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // --- Setup Response Header Parsing ---
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // --- Setup Response Body Parsing ---
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[](char *_ptr, size_t _size, size_t _nmemb, void *_userdata) -> size_t {
        auto *str = static_cast<std::string *>(_userdata);
        str->append(_ptr, _size * _nmemb);
        return _size * _nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);


    if (printHttpTrace) {
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, BackendConnection::debugCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }


    // Apply method-specific configurations
    if (configureMethod) {
        configureMethod(curl);
    }

    auto result = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
        return make_shared<BackendCurlError>(result, curl_easy_strerror(result));;
    }

    uint64_t statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    httpStatusCode = statusCode;
    if (httpStatusCode >= 400) {
        spdlog::error("Upstream service returned HTTP error: {}", httpStatusCode);
        return make_shared<BackendHttpError>(httpStatusCode);
    }

    return nullptr;
}

ptr<IBackendError> BackendConnection::proxyToBackEndGet(const string &url,
                                                        const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                        uint64_t &httpStatusCode,
                                                        vector<pair<string, string> > &responseHeaders,
                                                        std::string &responseBody, bool printHttpTrace) {
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody, nullptr,
        printHttpTrace);
}

ptr<IBackendError> BackendConnection::proxyToBackEndPost(const string &url,
                                                         const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                         const std::string &requestBody,
                                                         uint64_t &httpStatusCode,
                                                         vector<pair<string, string> > &responseHeaders,
                                                         std::string &responseBody, bool printHttpTrace) {
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody,
                              [&](CURL *curl) {
                                  curl_easy_setopt(curl, CURLOPT_POST, 1L);
                                  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
                                  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestBody.size()));
                              }, printHttpTrace);
}

ptr<IBackendError> BackendConnection::proxyToBackEndHead(const string &url,
                                                         const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                         uint64_t &httpStatusCode,
                                                         vector<pair<string, string> > &responseHeaders, bool printHttpTrace) {
    std::string responseBody; // Ignored for HEAD
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody,
                              [](CURL *curl) {
                                  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
                              }, printHttpTrace);
}

ptr<IBackendError> BackendConnection::proxyToBackEndOptions(const string &url,
                                                            const std::unique_ptr<proxygen::HTTPMessage> &
                                                            requestHeaders,
                                                            uint64_t &httpStatusCode,
                                                            vector<pair<string, string> > &responseHeaders,
                                                            std::string &responseBody, bool printHttpTrace) {
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody,
                              [](CURL *curl) {
                                  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                              }, printHttpTrace);
}

ptr<IBackendError> BackendConnection::proxyToBackEndPut(const string &url,
                                                        const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                        const std::string &requestBody,
                                                        uint64_t &httpStatusCode,
                                                        vector<pair<string, string> > &responseHeaders,
                                                        std::string &responseBody, bool printHttpTrace) {
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody,
                              [&](CURL *curl) {
                                  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                                  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
                                  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestBody.size()));
                              }, printHttpTrace);
}

ptr<IBackendError> BackendConnection::proxyToBackEndDelete(const string &url,
                                                           const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                           uint64_t &httpStatusCode,
                                                           vector<pair<string, string> > &responseHeaders,
                                                           std::string &responseBody, bool printHttpTrace) {
    return executeCurlRequest(url, requestHeaders, httpStatusCode, responseHeaders, responseBody,
                              [](CURL *curl) {
                                  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                              }, printHttpTrace);
}


curl_slist *BackendConnection::createCurlHeadersFromProxygen(
    const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders) {
    curl_slist *chunk = nullptr;

    // Block list (must be all lowercase)
    static const std::unordered_set<std::string> blockedHeaders = {
        "host",
        "content-length",
        "transfer-encoding",
        "connection",
        "expect",
        "keep-alive",
        "proxy-authenticate",
        "proxy-authorization",
        "upgrade"
    };

    if (requestHeaders) {
        requestHeaders->getHeaders().forEach(
            [&chunk](const std::string &name, const std::string &value) {
                std::string lowerName = name;
                folly::toLowerAscii(lowerName);

                if (blockedHeaders.find(lowerName) == blockedHeaders.end()) {
                    std::string headerStr = name + ": " + value;
                    chunk = curl_slist_append(chunk, headerStr.c_str());
                    CHECK_STATE(chunk);
                }
            });
    }


    return chunk;
}


std::string BackendConnection::trimWhiteSpaceFromHeader(const std::string &str) {
    return folly::trimWhitespace(str).str();
}


size_t BackendConnection::headerCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t totalSize = size * nitems;
    std::string raw(buffer, totalSize);

    auto *headers = static_cast<std::vector<std::pair<std::string, std::string> > *>(userdata);

    // Ignore blank lines (e.g., the one separating headers from body)
    if (folly::trimWhitespace(raw).empty()) {
        return totalSize;
    }

    // Ignore the HTTP status line (e.g., "HTTP/1.1 200 OK")
    if (raw.rfind("HTTP/", 0) == 0) {
        return totalSize;
    }

    // HTTP headers come in the format "Key: Value"
    size_t colonPos = raw.find(':');
    if (colonPos != std::string::npos) {
        std::string key = trimWhiteSpaceFromHeader(raw.substr(0, colonPos));
        std::string value = trimWhiteSpaceFromHeader(raw.substr(colonPos + 1));

        if (!key.empty()) {
            headers->emplace_back(key, value);
        }
    } else if (!headers->empty() && (raw.starts_with(' ') || raw.starts_with('\t'))) {
        // This is a folded header (continuation of the previous line)
        headers->back().second.append(" " + trimWhiteSpaceFromHeader(raw));
    }

    return totalSize;
}
