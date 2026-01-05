#pragma once
#include "../x402_protocol/IResponseSender.h"
#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <chrono>

class ProxygenResponseSender : public IResponseSender {
public:
    void sendResponse(const std::pair<uint16_t, std::string> &statusAndMessage,
                      const proxygen::HTTPHeaders &headers,
                      const std::string &body) override {
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

    static ptr<ProxygenResponseSender> makeShared(proxygen::ResponseHandler *downstream,
                                                  folly::EventBase *eventBase, proxygen::HTTPMessage &requestHeaders) {
        auto sender = ptr<ProxygenResponseSender>(new ProxygenResponseSender(downstream, eventBase, requestHeaders));
        sender->setWeakSelf(sender);
        return sender;
    }

private:
    proxygen::ResponseHandler *downstream_;
    folly::EventBase *eventBase_;
    weak_ptr<ProxygenResponseSender> weakSelf_;
    std::chrono::steady_clock::time_point creationTime_;
    proxygen::HTTPMessage requestHeaders_;

    explicit ProxygenResponseSender(proxygen::ResponseHandler *downstream,
                                    folly::EventBase *eventBase, proxygen::HTTPMessage &requestHeaders)
        : downstream_(downstream), eventBase_(eventBase), creationTime_(std::chrono::steady_clock::now()),
          requestHeaders_(requestHeaders) {
        CHECK_STATE(eventBase_);
        CHECK_STATE(downstream);
    }

    void setWeakSelf(const weak_ptr<ProxygenResponseSender> &weakSelf) {
        weakSelf_ = weakSelf;
    }
};
