#pragma once

#include <string>

class BackendError;

namespace proxygen {
    class HTTPMessage;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static ptr<BackendError>proxyToBackEndGet(const string &url,
                                                    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                    vector<pair<string, string> > &responseHeaders,
                                                    std::string &responseBody);

    static ptr<BackendError>proxyToBackEndPost(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     const std::string &requestBody,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody);

    static ptr<BackendError>proxyToBackEndHead(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                     vector<pair<string, string> > &responseHeaders);

    static ptr<BackendError>proxyToBackEndOptions(const string &url,
                                                        const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                        vector<pair<string, string> > &responseHeaders,
                                                        std::string &responseBody);

    static ptr<BackendError>proxyToBackEndPut(const string &url,
                                                    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                    const std::string &requestBody,
                                                    vector<pair<string, string> > &responseHeaders,
                                                    std::string &responseBody);

    static ptr<BackendError>proxyToBackEndDelete(const string &url,
                                                       const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                       vector<pair<string, string> > &responseHeaders,
                                                       std::string &responseBody);

    static curl_slist *createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:
    static ptr<BackendError>proxyToBackEnd(const string &url, proxygen::HTTPMethod method_,
                                                 const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                 const std::string &requestBody,
                                                 vector<pair<string, string> > &responseHeaders,
                                                 std::string &responseBody);

    static ptr<BackendError>executeCurlRequest(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody,
                                                     const std::function<void(CURL *)> &configureMethod);
};
