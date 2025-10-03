#include "common.h"
#include "X402Handler.h"
#include <folly/json.h>
#include <curl/curl.h>
#include "examples/PaymentExamples.h"

using namespace proxygen;

namespace {
    // Demo payment requirements JSON (normally dynamic / per-request).
    folly::dynamic paymentRequirements() {
        folly::dynamic req = folly::dynamic::object;
        auto paymentRequirements = EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA;
        req = folly::parseJson(paymentRequirements);
        return req;
    }
}


void X402Handler::onRequest(std::unique_ptr<HTTPMessage> _headers) noexcept {
    reqHeaders = std::move(_headers);
    auto path = reqHeaders->getPath();

    // Check for insecure path patterns

    // Reject empty or non-rooted paths
    if (path.empty() || path.front() != '/') {
        ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body("Invalid or insecure path")
            .sendWithEOM();
        return;
    }

    // Allow only [A-Za-z0-9] and '/'
    bool badChar = std::any_of(path.begin(), path.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '/');
    });

    if (badChar) {
        ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body("Path contains invalid characters")
            .sendWithEOM();
        return;
    }

    // Optionally: reject traversal attempts
    if (path.find("..") != std::string::npos) {
        ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body("Path traversal not allowed")
            .sendWithEOM();
        return;
    }
}

void X402Handler::onBody(std::unique_ptr<folly::IOBuf> _body) noexcept {
    if (!_body) return;
    _body->coalesce();
    bodyBuffer.append(reinterpret_cast<const char *>(_body->data()), _body->length());
}

bool X402Handler::hasValidPaymentHeader(const HTTPMessage* _req, std::string& _paymentInfo) {
    // TODO: Parse and verify real X-PAYMENT header.
    // This stub accepts "demo-ok".
    const auto& headerTable = _req->getHeaders();

    std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
    if (payment.empty()) return false;

    if (payment == "demo-ok") {
        _paymentInfo = R"({"txHash":"0xabc123...","amount":"0.25","asset":"USDC","network":"base-1net"})";
        return true;
    }
    return false;
}

void X402Handler::reply402() {
    auto json = folly::toJson(paymentRequirements());
    ResponseBuilder(downstream_)
            .status(402, "Payment Required")
            .header("Content-Type", "application/json")
            .body(json)
            .sendWithEOM();
}


void X402Handler::proxyToBackEnd(std::string _settlementInfo) {
    static thread_local std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlThreadLocal(nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            LOG(ERROR) << "Could not initialize CURL object";
            ResponseBuilder(downstream_)
                    .status(502, "Bad Gateway")
                    .header("Content-Type", "text/plain")
                    .body("Could not initialize CURL object")
                    .sendWithEOM();
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
        // Failed to fetch content, reply with 502
        ResponseBuilder(downstream_)
                .status(502, "Bad Gateway")
                .header("Content-Type", "text/plain")
                .body("Failed to fetch content from upstream service.")
                .sendWithEOM();
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
    if (hasValidPaymentHeader(reqHeaders.get(), settlementInfo)) {
        proxyToBackEnd(settlementInfo);
    } else {
        reply402();
    }
}
