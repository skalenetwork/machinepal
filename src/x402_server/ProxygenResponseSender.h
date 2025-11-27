#pragma once
#include "../x402_protocol/IResponseSender.h"
#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/ResponseBuilder.h>

class ProxygenResponseSender : public IResponseSender {
public:
    void sendResponse(const std::pair<uint16_t, std::string> &statusAndMessage,
                      const std::vector<std::pair<std::string, std::string> > &headers,
                      const std::string &body) override {
        auto task = [weakSelf = weakSelf_, statusAndMessage, headers, body]() mutable {
            auto self = weakSelf.lock();
            if (!self) {
                spdlog::error("Connection closed before sending reply");
                return;
            }
            proxygen::ResponseBuilder builder(self->downstream_);
            builder.status(statusAndMessage.first, statusAndMessage.second);
            for (const auto &h: headers) builder.header(h.first, h.second);
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
                                           folly::EventBase *eventBase) {
        auto sender = ptr<ProxygenResponseSender>(new ProxygenResponseSender(downstream, eventBase));
        sender->setWeakSelf(sender);
        return sender;
    }

private:

    proxygen::ResponseHandler *downstream_;
    folly::EventBase *eventBase_;
    weak_ptr<ProxygenResponseSender> weakSelf_;

    explicit ProxygenResponseSender(proxygen::ResponseHandler *downstream,
                                    folly::EventBase *eventBase)
        : downstream_(downstream), eventBase_(eventBase) {
        CHECK_STATE(eventBase_);
        CHECK_STATE(downstream);
    }

    void setWeakSelf(const weak_ptr<ProxygenResponseSender> &weakSelf) {
        weakSelf_ = weakSelf;
    }
};
