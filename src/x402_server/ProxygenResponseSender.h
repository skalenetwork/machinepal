#pragma once
#include <proxygen/httpserver/ResponseBuilder.h>
#include <string>
#include <vector>
#include "../x402_protocol/IResponseSender.h"
#include <folly/io/async/EventBaseManager.h>

class ProxygenResponseSender : public IResponseSender
{
public:
    explicit ProxygenResponseSender(proxygen::ResponseHandler* downstream)
        : downstream_(downstream),
          eventBase_(folly::EventBaseManager::get()->getEventBase())
    {
        CHECK_STATE(eventBase_);
        CHECK_STATE(downstream);
    }

    void sendResponse(const std::pair<uint16_t, std::string>& statusAndMessage,
                      const std::vector<std::pair<std::string, std::string>>& headers,
                      const std::string& body = "") override
    {
        proxygen::ResponseBuilder builder(downstream_);
        builder.status(statusAndMessage.first, statusAndMessage.second);
        for (const auto& h : headers)
        {
            builder.header(h.first, h.second);
        }
        if (!body.empty())
        {
            builder.body(body);
        }
        // Ensure sendWithEOM runs in the correct event base thread that handle this particular http connection
        if (folly::EventBaseManager::get()->getEventBase() == eventBase_)
        {
            builder.sendWithEOM();
        }
        else
        {
            eventBase_->runInEventBaseThread([builder = std::move(builder)]() mutable
            {
                builder.sendWithEOM();
            });
        }
    }

private:
    proxygen::ResponseHandler* downstream_;
    folly::EventBase* eventBase_;
};