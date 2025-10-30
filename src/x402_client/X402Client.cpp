#include "X402Client.h"

#include "payment/datastructures/PaymentPayload.h"

X402Client::X402Client(const std::string &_connect_ip, uint16_t _port)
    : connectHost(_connect_ip), port(_port) {}

X402Client::~X402Client() {}

std::string X402Client::baseUrl() {
    return "http://" + connectHost;
}

std::string X402Client::parseStatusLineAndHeaders(const std::vector<std::string> &_headersVector,
                                                  std::map<std::string, std::string> &_headersMap) {
    std::string statusLine;
    if (!_headersVector.empty()) {
        statusLine = _headersVector[0];
        statusLine.erase(statusLine.find_last_not_of(" \t\r\n") + 1);
    }
    for (size_t _i = 1; _i < _headersVector.size(); ++_i) {
        auto _pos = _headersVector[_i].find(':');
        if (_pos != std::string::npos) {
            std::string _key = _headersVector[_i].substr(0, _pos);
            std::string _value = _headersVector[_i].substr(_pos + 1);
            _key.erase(0, _key.find_first_not_of(" \t\r\n"));
            _key.erase(_key.find_last_not_of(" \t\r\n") + 1);
            _value.erase(0, _value.find_first_not_of(" \t\r\n"));
            _value.erase(_value.find_last_not_of(" \t\r\n") + 1);
            _headersMap[_key] = _value;
        }
    }
    return statusLine;
}

std::tuple<std::map<std::string, std::string>, std::string, HttpResponse>
X402Client::sendRequestAndParseResult(std::string _location, const std::vector<std::string> &_extraHeaders,
    bool printHttpTrace ) {
    auto resp = httpGet(baseUrl(), _location, _extraHeaders, printHttpTrace);
    auto headersVector = resp.headers;
    std::map<std::string, std::string> headersMap;
    auto statusLine = parseStatusLineAndHeaders(headersVector, headersMap);
    spdlog::info("STATUS::{}", statusLine);
    for (const auto &[key, value]: headersMap) {
        spdlog::info("{}: {}", key, value);
    }
    spdlog::info("BODY::{}", resp.body);
    return {headersMap, statusLine, resp};
}


std::tuple<std::map<std::string, std::string>, std::string, HttpResponse>
X402Client::sendRequestWithPayloadAndParseResult(std::string _location, ptr<PaymentPayload> payload,
    bool printHttpTrace ) {
    CHECK_STATE(payload);
    auto header = payload->createHttpHeaderValue();
    return sendRequestAndParseResult(_location, {header}, printHttpTrace);
}
size_t X402Client::writeBody(char *_ptr, size_t _size, size_t _nmemb, void *_userdata) {
    auto *out = static_cast<std::string *>(_userdata);
    out->append(_ptr, _size * _nmemb);
    return _size * _nmemb;
}

size_t X402Client::writeHeader(char *_buffer, size_t _size, size_t _nitems, void *_userdata) {
    auto *headers = static_cast<std::vector<std::string> *>(_userdata);
    headers->emplace_back(_buffer, _size * _nitems);
    return _size * _nitems;
}

int X402Client::debugCallback(CURL *, curl_infotype type, char *data, size_t size, void *) {
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

HttpResponse X402Client::httpGet(const std::string &_baseURL, const std::string &_location,
                                 const std::vector<std::string> &_extraHeaders, bool printHttpTrace) {
    CURL *curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string url = _baseURL + ":" + std::to_string(port) + _location;

    struct curl_slist *hdrs = nullptr;
    for (auto &_h: _extraHeaders) hdrs = curl_slist_append(hdrs, _h.c_str());

    HttpResponse resp;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp.headers);

    if (printHttpTrace)
    {
        // Then in your httpGet function, add before curl_easy_perform:
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debugCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    if (hdrs)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    auto rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        if (hdrs) curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("curl perform error: ") + curl_easy_strerror(rc));
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status);

    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return resp;
}