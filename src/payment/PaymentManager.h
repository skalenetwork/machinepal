#pragma once

#include <memory>
#include <optional>
#include "x402_protocol/HttpError.h"

class HttpError;

namespace proxygen {
    class HTTPMessage;
}

class MachinePayApp; // forward declaration

class PaymentManager {
public:
    explicit PaymentManager(MachinePayApp& app);
    [[nodiscard]] MachinePayApp& app() const { return app_; }

    std::optional<HttpError> validatePayment(
        const std::unique_ptr<proxygen::HTTPMessage> &req, std::string &settlementInfo);
private:
    MachinePayApp& app_;
};
