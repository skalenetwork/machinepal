#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "datastructures/PaymentPayload.h"
#include "x402_protocol/HttpError.h"

class HttpError;

namespace proxygen {
    class HTTPMessage;
}

class MachinePayApp; // forward declaration
class MachinePayConfig; // forward declaration
class ResourceConfig;   // forward declaration

class PaymentManager {
public:
    explicit PaymentManager(MachinePayApp& app);

    variant<ptr<PaymentPayload>, HttpError> decodeAndParsePayment(const std::unique_ptr<proxygen::HTTPMessage> &req);

    std::optional<HttpError> validatePaymentPayload(
        const MachinePayConfig &config, const ResourceConfig &resource, shared_ptr<PaymentPayload> paymentPayload);


    [[nodiscard]] MachinePayApp& app() const { return app_; }

    void recordSuccessfulSettlement(const shared_ptr<PaymentPayload> & payload, const ResourceConfig & resource);

    std::optional<HttpError>  checkAgaistAlreadySettledPayments(const shared_ptr<PaymentPayload> & shared, const ResourceConfig & resource);

    std::optional<HttpError> decodeValidateAndSettlePayment(
        const std::unique_ptr<proxygen::HTTPMessage> &req,
        std::string &settlementInfo,
        const MachinePayConfig& config,
        const ResourceConfig& resource);
private:
    MachinePayApp& app_;
};
