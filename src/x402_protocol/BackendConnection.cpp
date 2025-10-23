//
// Created by stan on 11/10/25.
//
#include "MachinepayCommon.h"
#include "BackendConnection.h"
#include "X402Processor.h"
#include "curl/curl.h"

bool BackendConnection::proxyToBackEnd(std::string& backendResponseBody, std::string& errorMessage) {
    static thread_local std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlThreadLocal(nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            spdlog::error("Could not initialize CURL object");
            errorMessage =  "Could not initialize CURL object";
            return false;
        }
        curlThreadLocal.reset(curlObject);
    }
    // Fetch content from the external URL
    CHECK_STATE(curlThreadLocal);
    auto* curl = curlThreadLocal.get();
    curl_easy_reset(curl);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     +[](char* _ptr, size_t _size, size_t _nmemb, void* _userdata) -> size_t {
                     auto* str = static_cast<std::string*>(_userdata);
                     str->append(_ptr, _size * _nmemb);
                     return _size * _nmemb;
                     });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &backendResponseBody);

    auto result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        spdlog::error("CURL error: {}", curl_easy_strerror(result));
    }

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        errorMessage =  "Failed to fetch content from upstream service.";
        return false;
    } else
    {
        return true;
    }
}
