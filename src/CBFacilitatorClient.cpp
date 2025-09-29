#include "common.h"
#include "CBFacilitatorClient.h"

#include <stdexcept>
#include <mutex>
#include <curl/curl.h>

#include "exceptions/VerificationError.h"

CBFacilitatorClient::CBFacilitatorClient(
    std::string _base_url,
    std::string _auth,
    long _connect_timeout_ms,
    long _total_timeout_ms)
    : base_url(std::move(_base_url)),
      authHeaderValue(std::move(_auth)),
      connect_timeout_ms(_connect_timeout_ms),
      total_timeout_ms(_total_timeout_ms) {
    ensureCurlGlobalInit();
}

void CBFacilitatorClient::ensureCurlGlobalInit() {
    static std::once_flag once;
    std::call_once(once, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
}

size_t CBFacilitatorClient::writeCallback(char * _ptr, size_t _size, size_t _nmemb, void * _userdata) {
    const size_t real_size = _size * _nmemb;
    auto *buf = static_cast<std::string *>(_userdata);
    buf->append(_ptr, real_size);
    return real_size;
}

std::string CBFacilitatorClient::joinUrl(const std::string &_base, const std::string &_path) {
    if (_base.empty()) return _path;
    if (_path.empty()) return _base;
    const bool b = _base.back() == '/';
    const bool p = _path.front() == '/';
    if (b && p) return _base + _path.substr(1);
    if (!b && !p) return _base + "/" + _path;
    return _base + _path;
}

const std::string USDC_SEPOLIA_ADDRESS = "0x036CbD53842c5426634e7929541eC2318f3dCF7e";

const std::string VERIFY_PAYLOAD_EXAMPLE = R"JSON(
{
    "x402Version": 1,
    "paymentPayload": {
        "x402Version": 1,
        "scheme": "exact",
        "network": "base-sepolia",
        "payload": {
            "signature": "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef1b",
            "authorization": {
                "from": "0x1111111111111111111111111111111111111111",
                "to": "0x2222222222222222222222222222222222222222",
                "value": "1000",
                "validAfter": "1716150000",
                "validBefore": "1716153600",
                "nonce": "0x1234567890abcdef"
            }
        }
    },
    "paymentRequirements": {
        "scheme": "exact",
        "network": "base-sepolia",
        "maxAmountRequired": "1000",
        "resource": "https://api.example.com/premium/data",
        "description": "Test API data",
        "mimeType": "application/json",
        "payTo": "0x2222222222222222222222222222222222222222",
        "maxTimeoutSeconds": 10,
        "asset": "0x036CbD53842c5426634e7929541eC2318f3dCF7e"
    }
}
)JSON";


nlohmann::json CBFacilitatorClient::verify(const nlohmann::json &_paymentInstruction,
                                           const nlohmann::json &_paymentPayload) const {
    nlohmann::json body;
    body["paymentInstruction"] = _paymentInstruction;
    body["paymentPayload"] = _paymentPayload;
    return postJson("/verify", body);
}

nlohmann::json CBFacilitatorClient::settle(const nlohmann::json &_paymentInstruction,
                                           const nlohmann::json &_paymentPayload) const {
    nlohmann::json body;
    body["paymentInstruction"] = _paymentInstruction;
    body["paymentPayload"] = _paymentPayload;
    return postJson("/settle", body);
}

std::string CBFacilitatorClient::extractInvalidReason(std::string &_responseData) const {
    try {
        auto errJson = nlohmann::json::parse(_responseData);
        if (errJson.contains("reason") && errJson["reason"].is_string()) {
            return errJson["reason"].get<std::string>();
        }
    } catch (...) {
        // Ignore JSON parse errors here
    }
    return {};
}

nlohmann::json CBFacilitatorClient::postJson(const std::string &_path, const nlohmann::json &_body) const {
    const std::string url = joinUrl(base_url, _path);
    std::string payload = _body.dump();

    payload = VERIFY_PAYLOAD_EXAMPLE;

    CURL *curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init CURL easy handle");

    std::string responseData;
    long httpCode = 0;

    // Build headers
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!authHeaderValue.empty()) {
        std::string auth = "Authorization: " + authHeaderValue;
        headers = curl_slist_append(headers, auth.c_str());
    }
    for (const auto &h: extraHeaders) {
        headers = curl_slist_append(headers, h.c_str());
    }

    // Set options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CBFacilitatorClient/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CBFacilitatorClient::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, total_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (!proxyUrl.empty()) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyUrl.c_str());
    }

    // Perform
    CURLcode res = curl_easy_perform(curl);

    // Collect HTTP code before cleanup
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    // Cleanup
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }


    if (httpCode < 200 || httpCode >= 300) {
        std::string errorExplanation;
        switch (httpCode) {
            case 400: {
                errorExplanation = "Bad Request";
                std::string invalidReason = extractInvalidReason(responseData);
                errorExplanation += ":" + invalidReason;
                throw VerificationError(errorExplanation);
                break;
            }
            case 401: errorExplanation = "Unauthorized";
                break;
            case 403: errorExplanation = "Forbidden";
                break;
            case 404: errorExplanation = "Not Found";
                break;
            case 500: errorExplanation = "Internal Server Error";
                break;
            case 502: errorExplanation = "Bad Gateway";
                break;
            case 503: errorExplanation = "Service Unavailable";
                break;
            default: errorExplanation = "Unknown Error";
                break;
        }
        LOG(ERROR);
        throw std::runtime_error("HTTP " + std::to_string(httpCode) + " (" + errorExplanation + ") error at "
                                 + url + ": " + responseData + "\n | Payload: " + payload);
    }

    if (responseData.empty()) {
        throw std::runtime_error("HTTP server at " + url + " returned empty response (HTTP " +
                                 std::to_string(httpCode) + ")");
    }

    // Parse response
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(responseData);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse JSON (HTTP " + std::to_string(httpCode) +
                                 ") from " + url + ": " + std::string(e.what()) + " | Raw: " + responseData);
    }


    return j;
}
