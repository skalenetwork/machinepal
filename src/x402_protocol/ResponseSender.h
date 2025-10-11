#pragma once
#include <proxygen/httpserver/ResponseBuilder.h>
#include <string>
#include <vector>

class ResponseSender {
public:
    explicit ResponseSender(proxygen::ResponseHandler* downstream)
        : downstream_(downstream)
    {
        CHECK_STATE(downstream);
    }

    void sendResponse(const std::pair<uint16_t, std::string>& statusAndMessage,
                      const std::vector<std::pair<std::string, std::string>>& headers,
                      const std::string& body = "")
    {
        proxygen::ResponseBuilder builder(downstream_);
        builder.status(statusAndMessage.first, statusAndMessage.second);
        for (const auto& h : headers) {
            builder.header(h.first, h.second);
        }
        if (!body.empty()) {
            builder.body(body);
        }
        builder.sendWithEOM();
    }

private:
    proxygen::ResponseHandler* downstream_;
};
