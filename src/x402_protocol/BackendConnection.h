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
                                                    std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError>proxyToBackEndPost(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     const std::string &requestBody,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError>proxyToBackEndHead(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders, bool printHttpTrace = false);

    static ptr<IBackendError>proxyToBackEndOptions(const string &url,
                                                        const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                        uint64_t& httpStatusCode,
                                                        vector<pair<string, string> > &responseHeaders,
                                                        std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError>proxyToBackEndPut(const string &url,
                                                    const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                    const std::string &requestBody,
                                                    uint64_t& httpStatusCode,
                                                    vector<pair<string, string> > &responseHeaders,
                                                    std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError>proxyToBackEndDelete(const string &url,
                                                       const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                       uint64_t& httpStatusCode,
                                                       vector<pair<string, string> > &responseHeaders,
                                                       std::string &responseBody, bool printHttpTrace = false);

    static curl_slist *createCurlHeadersFromProxygen(const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders);

    static std::string trimWhiteSpaceFromHeader(const std::string &str);

    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);

public:
    static ptr<IBackendError>proxyToBackEnd(const string &url, proxygen::HTTPMethod method_,
                                                 const std::unique_ptr<proxygen::HTTPMessage> &reqHeaders,
                                                 const std::string &requestBody,
                                                 uint64_t& httpStatusCode,
                                                 vector<pair<string, string> > &responseHeaders,
                                                 std::string &responseBody, bool printHttpTrace = false);

    static ptr<IBackendError>executeCurlRequest(const string &url,
                                                     const std::unique_ptr<proxygen::HTTPMessage> &requestHeaders,
                                                     uint64_t& httpStatusCode,
                                                     vector<pair<string, string> > &responseHeaders,
                                                     std::string &responseBody,
                                                     const std::function<void(CURL *)> &configureMethod,
                                                     bool printHttpTrace);

    static int debugCallback( CURL*, curl_infotype type, char* data, size_t size, void* ) {
        switch ( type ) {
            case CURLINFO_HEADER_OUT:
                spdlog::info( "CURL SEND HEADER:\n{}", std::string( data, size ) );
                break;
            case CURLINFO_DATA_OUT:
                spdlog::info( "CURL SEND DATA:\n{}", std::string( data, size ) );
                break;
            case CURLINFO_HEADER_IN:
                spdlog::info( "CURL RECV HEADER:\n{}", std::string( data, size ) );
                break;
            case CURLINFO_DATA_IN:
                spdlog::info( "CURL RECV DATA:\n{}", std::string( data, size ) );
                break;
            default:
                break;
        }
        return 0;
    }
};
