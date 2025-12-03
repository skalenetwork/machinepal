#pragma once

#include <string>

class IBackendError;

namespace proxygen {
    class HTTPHeaders;
    class HTTPHeaders;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class HttpEndpointConnection {
    static curl_slist *createCurlHeadersFromProxygenHeaders(
        const proxygen::HTTPHeaders &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:
    static ptr<IBackendError> doGetRequest(const string &url,
                                           const proxygen::HTTPHeaders &reqHeaders,
                                           uint64_t &httpStatusCode,
                                           proxygen::HTTPHeaders &responseHeaders,
                                           std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> doPostRequest(const string &url,
                                            const proxygen::HTTPHeaders &requestHeaders,
                                            const std::string &requestBody,
                                            uint64_t &httpStatusCode,
                                            proxygen::HTTPHeaders &responseHeaders,
                                            std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> doHeadRequest(const string &url,
                                            const proxygen::HTTPHeaders &reqHeaders,
                                            uint64_t &httpStatusCode,
                                            proxygen::HTTPHeaders &responseHeaders, bool printHttpTrace = false);

    static ptr<IBackendError> doOptions(const string &url,
                                        const proxygen::HTTPHeaders &reqHeaders,
                                        uint64_t &httpStatusCode,
                                        proxygen::HTTPHeaders &responseHeaders,
                                        std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> doPutRequest(const string &url,
                                           const proxygen::HTTPHeaders &reqHeaders,
                                           const std::string &requestBody,
                                           uint64_t &httpStatusCode,
                                           proxygen::HTTPHeaders &responseHeaders,
                                           std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> doDeleteRequest(const string &url,
                                              const proxygen::HTTPHeaders &requestHeaders,
                                              uint64_t &httpStatusCode,
                                              proxygen::HTTPHeaders &responseHeaders,
                                              std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> doRequest(const string &url, proxygen::HTTPMethod method_,
                                        const proxygen::HTTPHeaders &reqHeaders,
                                        const std::string &requestBody,
                                        uint64_t &httpStatusCode,
                                        proxygen::HTTPHeaders &responseHeaders,
                                        std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError> executeCurlRequest(const string &url,
                                                 const proxygen::HTTPHeaders &requestHeaders,
                                                 uint64_t &httpStatusCode,
                                                 proxygen::HTTPHeaders &responseHeaders,
                                                 std::string &responseBody,
                                                 const std::function<void(CURL *)> &configureMethod,
                                                 bool printHttpTrace);

    static int debugCallback(CURL *, curl_infotype type, char *data, size_t size, void *) {
        switch (type) {
            case CURLINFO_HEADER_OUT:
                spdlog::info("CURL SEND HEADER:\n{}", std::string(data, size));
                break;
            case CURLINFO_DATA_OUT:
                spdlog::info("CURL SEND DATA:\n{}", std::string(data, size));
                break;
            case CURLINFO_HEADER_IN:
                spdlog::info("CURL RECV HEADER:\n{}", std::string(data, size));
                break;
            case CURLINFO_DATA_IN:
                spdlog::info("CURL RECV DATA:\n{}", std::string(data, size));
                break;
            default:
                break;
        }
        return 0;
    }
};
