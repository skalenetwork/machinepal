#include "MachinePalCommon.h"
#include "ProxygenResponseSender.h"


using namespace proxygen;

void ProxygenResponseSender::sendResponse(const std::pair<uint16_t, std::string> &statusAndMessage,
                                          const proxygen::HTTPHeaders &headers,
                                          const std::string &body) {
    auto task = [weakSelf = weakSelf_, statusAndMessage, headers, body]() mutable {
        auto self = weakSelf.lock();
        if (!self) {
            LOG_NETWORK_ERROR("Connection closed before sending reply");
            return;
        }
        proxygen::ResponseBuilder builder(self->downstream_);
        builder.status(statusAndMessage.first, statusAndMessage.second);
        headers.forEach([&builder](const std::string &name, const std::string &value) {
            builder.header(name, value);
        });
        if (!body.empty()) builder.body(body);
        builder.sendWithEOM();
    };

    if (folly::EventBaseManager::get()->getEventBase() == eventBase_) {
        task();
    } else {
        eventBase_->runInEventBaseThread(std::move(task));
    }
}

ptr<ProxygenResponseSender> ProxygenResponseSender::makeShared(proxygen::ResponseHandler *downstream,
                                              folly::EventBase *eventBase, proxygen::HTTPMessage &requestHeaders) {
    auto sender = ptr<ProxygenResponseSender>(new ProxygenResponseSender(downstream, eventBase, requestHeaders));
    sender->setWeakSelf(sender);
    return sender;
}

ProxygenResponseSender::ProxygenResponseSender(proxygen::ResponseHandler *downstream,
                                folly::EventBase *eventBase, proxygen::HTTPMessage &requestHeaders)
    : downstream_(downstream), eventBase_(eventBase), creationTime_(std::chrono::steady_clock::now()),
      requestHeaders_(requestHeaders) {
    CHECK_STATE(eventBase_);
    CHECK_STATE(downstream);
}

void ProxygenResponseSender::setWeakSelf(const weak_ptr<ProxygenResponseSender> &weakSelf) {
    weakSelf_ = weakSelf;
}

void ProxygenResponseSender::logAccess(
    const std::string& service,
    const std::string& method,
    const std::string& path,
    int status,
    uint64_t bytesSent,
    const std::string& clientIp,
    const std::string& userAgent,
    const std::string& requestId
) {

    auto now = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - creationTime_).count();
    auto systemNow = std::chrono::system_clock::now();
    std::time_t systemNowTime = std::chrono::system_clock::to_time_t(systemNow);
    std::tm tm{};
    gmtime_r(&systemNowTime, &tm);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm);

    LOG_ACCESS_INFO(
        "{{"
        "\"timestamp\":\"{}\","
        "\"level\":\"info\","
        "\"type\":\"access\","
        "\"service\":\"{}\","
        "\"method\":\"{}\","
        "\"path\":\"{}\","
        "\"status\":{},"
        "\"bytesSent\":{},"
        "\"latencyMs\":{},"
        "\"clientIp\":\"{}\","
        "\"userAgent\":\"{}\","
        "\"requestId\":\"{}\""
        "}}",
        timestamp,
        service,
        method,
        path,
        status,
        bytesSent,
        durationMs,
        clientIp,
        userAgent,
        requestId
    );
}