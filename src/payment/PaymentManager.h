#pragma once

#include "config/subconfigs/NetworkConfig.h"

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

    void recordSuccessfulSettlement(const PaymentPayload & payload, const EIP712Domain& domain,
        const ResourceConfig & resource);

    std::optional<HttpError>  checkAgaistAlreadySettledPayments(const ptr<PaymentPayload> & paymentPayload,
        const ptr<EIP712Domain>& domain);

    bool markPaymentAsBeingSettled(ptr<Authorization> authorization, ptr<EIP712Domain> domain);
    void unmarkPaymentAsBeingSettled(ptr<Authorization> _authorization, ptr<EIP712Domain> _domain);

    std::optional<HttpError> settle(std::string &settlementInfo, const NetworkConfig &networkConfig,
                        const ResourceConfig &resource,
                       shared_ptr<PaymentPayload> paymentPayload);


    std::optional<HttpError> decodeValidateAndSettlePayment(
        const std::unique_ptr<proxygen::HTTPMessage> &req,
        std::string &settlementInfo,
        const MachinePayConfig& config,
        const ResourceConfig& resource);
private:
    MachinePayApp& app_;

    std::set<std::string> currentlySettlingPayments_;
    std::mutex currentlySettlingPaymentsMutex_;

};
