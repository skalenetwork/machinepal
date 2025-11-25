#pragma once

#include <string>

namespace proxygen {
    class HTTPMessage;
    enum class HTTPMethod;
}

class X402Processor;
class IResponseSender;

class BackendConnection {
    static bool proxyToBackEndGet(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                  vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                  std::string &errorMessage);

    static bool proxyToBackEndPost(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                            const std::string &requestBody,
                            vector<pair<string, string>> &responseHeaders, std::string &responseBody, std::string &errorMessage);

    static bool proxyToBackEndHead(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                   vector<pair<string, string>> &responseHeaders,
                                   std::string &errorMessage);

    static bool proxyToBackEndOptions(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                      vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                      std::string &errorMessage);

    static bool proxyToBackEndPut(const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                  const std::string &requestBody,
                                  vector<pair<string, string>> &responseHeaders, std::string &responseBody,
                                  std::string &errorMessage);

    static curl_slist *createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:

    static bool proxyToBackEnd(proxygen::HTTPMethod method_, const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                               const std::string &requestBody, vector<pair<string, string>> &responseHeaders,
                               std::string &responseBody, std::string &errorMessage);
};
