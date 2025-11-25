#pragma once

#include <string>

namespace proxygen {
    class HTTPMessage;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static bool proxyToBackEndGet(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                  vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                  uint64_t& errorCode,
                                  std::string &errorMessage);

    static bool proxyToBackEndPost(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                            const std::string &requestBody,
                            vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                            uint64_t& errorCode, std::string &errorMessage);

    static bool proxyToBackEndHead(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                   vector<pair<string, string>> &responseHeaders,
                                   uint64_t& errorCode, std::string &errorMessage);

    static bool proxyToBackEndOptions(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                      vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                      uint64_t& errorCode, std::string &errorMessage);

    static bool proxyToBackEndPut(const string& url, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                  const std::string &requestBody,
                                  vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                  uint64_t& errorCode, std::string &errorMessage);

    static bool proxyToBackEndDelete(const string &url, const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                              vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                              uint64_t &errorCode,
                              std::string &errorMessage);

    static curl_slist *createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:

    static bool proxyToBackEnd(const string& url, proxygen::HTTPMethod method_, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                               const std::string &requestBody, vector<pair<string, string>> &responseHeaders,
                               std::string &responseBody, uint64_t& errorCode, std::string &errorMessage);

    static bool executeCurlRequest(const string &url, const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                            vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                            uint64_t &errorCode,
                            std::string &errorMessage, const std::function<void(CURL *)> &configureMethod);
};
