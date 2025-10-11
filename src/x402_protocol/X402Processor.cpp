#include "common.h"
#include "X402Processor.h"
#include "MachinePayApp.h"
#include <proxygen/httpserver/ResponseBuilder.h>
#include <folly/json.h>
#include "examples/PaymentExamples.h"




X402Processor::X402Processor(MachinePayApp& app)
    : app_(app)
{
    // TODO: Add any initialization logic if needed
}

bool X402Processor::hasValidPaymentHeader(const proxygen::HTTPMessage* _req, std::string& _paymentInfo) {
    // TODO: Parse and verify real X-PAYMENT header.
    // This stub accepts "demo-ok".
    const auto& headerTable = _req->getHeaders();
    std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
    if (payment.empty()) return false;
    if (payment == "demo-ok") {
        _paymentInfo = R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"USDC\",\"network\":\"base-1net\"})";
        return true;
    }
    return false;
}



void X402Processor::reply200(proxygen::ResponseHandler* downstream, const std::string& settlementInfo, std::string proxyBody)
{
    proxygen::ResponseBuilder(downstream)
        .status(200, "OK")
        .header("Content-Type", "text/plain")
        .header("X-PAYMENT-RESPONSE", settlementInfo)
        .body(proxyBody)
        .sendWithEOM();
}


void X402Processor::reply402(proxygen::ResponseHandler* downstream) {
    // Demo payment requirements JSON (normally dynamic / per-request).
    folly::dynamic req = folly::dynamic::object;
    auto paymentRequirements = EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA;
    req = folly::parseJson(paymentRequirements);
    auto json = folly::toJson(req);
    proxygen::ResponseBuilder(downstream)
        .status(402, "Payment Required")
        .header("Content-Type", "application/json")
        .body(json)
        .sendWithEOM();
}

void X402Processor::reply400(proxygen::ResponseHandler* downstream, const std::string& message) {
    proxygen::ResponseBuilder(downstream)
        .status(400, "Bad Request")
        .body(message)
        .sendWithEOM();
}


void X402Processor::reply502(proxygen::ResponseHandler* downstream, const std::string& message) {
    proxygen::ResponseBuilder(downstream)
        .status(502, "Bad Gateway")
        .header("Content-Type", "text/plain")
        .body(message)
        .sendWithEOM();
}

bool X402Processor::processUrlAndHeaders(const proxygen::HTTPMessage* reqHeaders, proxygen::ResponseHandler* downstream) {
    auto path = reqHeaders->getPath();
    // Reject empty or non-rooted paths
    if (path.empty() || path.front() != '/') {
        reply400(downstream, "Invalid or insecure path");
        return true;
    }
    // Allow only [A-Za-z0-9] and '/'
    bool badChar = std::any_of(path.begin(), path.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '/');
    });
    if (badChar) {
        reply400(downstream, "Path contains invalid characters");
        return true;
    }
    // Optionally: reject traversal attempts
    if (path.find("..") != std::string::npos) {
        reply400(downstream, "Path traversal not allowed");
        return true;
    }
    return false;
}


void X402Processor::proxyToBackEnd(proxygen::ResponseHandler* downstream, const std::string& settlementInfo) {
    static thread_local std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlThreadLocal(nullptr, &curl_easy_cleanup);

    if (!curlThreadLocal) {
        auto curlObject = curl_easy_init();
        if (!curlObject) {
            LOG(ERROR) << "Could not initialize CURL object";
            reply502(downstream, "Could not initialize CURL object");
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
        reply502(downstream, "Failed to fetch content from upstream service.");
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    reply200(downstream, settlementInfo, proxyBody);
}
