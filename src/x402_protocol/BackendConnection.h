#pragma once

#include <string>

class IBackendError;

namespace proxygen {
    class HTTPMessage;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static ptr<IBackendError>proxyToBackEndGet(const string &url,
                                                    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                    uint64_t& httpStatusCode,
                                                    vector<pair<string, string> > &responseHeaders,
                                                    std::string &responseBody);

    static ptr<IBackendError>proxyToBackEndPost(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     const std::string &requestBody,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody);

    static ptr<IBackendError>proxyToBackEndHead(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders);

    static ptr<IBackendError>proxyToBackEndOptions(const string &url,
                                                        const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                        uint64_t& httpStatusCode,
                                                        vector<pair<string, string> > &responseHeaders,
                                                        std::string &responseBody);

    static ptr<IBackendError>proxyToBackEndPut(const string &url,
                                                    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                    const std::string &requestBody,
                                                    uint64_t& httpStatusCode,
                                                    vector<pair<string, string> > &responseHeaders,
                                                    std::string &responseBody);

    static ptr<IBackendError>proxyToBackEndDelete(const string &url,
                                                       const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                       uint64_t& httpStatusCode,
                                                       vector<pair<string, string> > &responseHeaders,
                                                       std::string &responseBody);

    static curl_slist *createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:
    static ptr<IBackendError>proxyToBackEnd(const string &url, proxygen::HTTPMethod method_,
                                                 const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                 const std::string &requestBody,
                                                 uint64_t& httpStatusCode,
                                                 vector<pair<string, string> > &responseHeaders,
                                                 std::string &responseBody);

    static ptr<IBackendError>executeCurlRequest(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody,
                                                     const std::function<void(CURL *)> &configureMethod);
};
