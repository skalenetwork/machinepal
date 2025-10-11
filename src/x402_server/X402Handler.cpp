#include "common.h"
#include "X402Handler.h"
#include <folly/json.h>
#include <curl/curl.h>
#include "examples/PaymentExamples.h"

using namespace proxygen;


void X402Handler::reply400(const std::string& message)
{
    ResponseBuilder(downstream_)
        .status(400, "Bad Request")
        .body(message)
        .sendWithEOM();
}

void X402Handler::onRequest(std::unique_ptr<HTTPMessage> _headers) noexcept {
    reqHeaders_ = std::move(_headers);
    auto path = reqHeaders_->getPath();

    // Check for insecure path patterns

    // Reject empty or non-rooted paths
    if (path.empty() || path.front() != '/') {
        reply400("Invalid or insecure path");
        return;
    }

    // Allow only [A-Za-z0-9] and '/'
    bool badChar = std::any_of(path.begin(), path.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '/');
    });

    if (badChar) {
        reply400("Path contains invalid characters");
        return;
    }

    // Optionally: reject traversal attempts
    if (path.find("..") != std::string::npos) {
        reply400("Path traversal not allowed");
        return;
    }
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    if (!_body) return;
    _body->coalesce();
    bodyBuffer_.append(reinterpret_cast<const char *>(_body->data()), _body->length());
}



void X402Handler::reply502(const std::string& message)
{
    ResponseBuilder(downstream_)
        .status(502, "Bad Gateway")
        .header("Content-Type", "text/plain")
        .body(message)
        .sendWithEOM();
}

void X402Handler::proxyToBackEnd(std::string _settlementInfo) {
    static thread_local std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlThreadLocal(nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            LOG(ERROR) << "Could not initialize CURL object";
            reply502("Could not initialize CURL object");
            return;
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

    std::string proxyBody;

    curl_easy_setopt(curl, CURLOPT_URL, "https://jsonplaceholder.typicode.com/posts/1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     +[](char* _ptr, size_t _size, size_t _nmemb, void* _userdata) -> size_t {
                     auto* str = static_cast<std::string*>(_userdata);
                     str->append(_ptr, _size * _nmemb);
                     return _size * _nmemb;
                     });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &proxyBody);

    auto result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        LOG(ERROR) << "CURL error: " << curl_easy_strerror(result);
        reply502("Failed to fetch content from upstream service.");
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    ResponseBuilder(downstream_)
            .status(200, "OK")
            .header("Content-Type", "text/plain")
            .header("X-PAYMENT-RESPONSE", _settlementInfo)
            .body(proxyBody)
            .sendWithEOM();
    return;
}

void X402Handler::onEOM() noexcept {
    std::string settlementInfo;
    if (X402Processor::hasValidPaymentHeader(reqHeaders_.get(), settlementInfo)) {
        proxyToBackEnd(settlementInfo);
    } else {
        X402Processor::reply402(downstream_);
    }
}
