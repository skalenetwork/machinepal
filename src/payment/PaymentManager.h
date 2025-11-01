#pragma once

#include "config/subconfigs/NetworkConfig.h"


#include "datastructures/PaymentPayload.h"
#include "datastructures/SettlementResponse.h"
#include "x402_protocol/HttpError.h"

class SettlementResponse;
class HttpError;

namespace proxygen {
    class HTTPMessage;
}

class MachinePayApp; // forward declaration
class MachinePayConfig; // forward declaration
class ResourceConfig; // forward declaration

class PaymentManager {
public:
    explicit PaymentManager(MachinePayApp &app);

    variant<ptr<PaymentPayload>, HttpError> decodeAndParsePayment(const std::unique_ptr<proxygen::HTTPMessage> &req);

    std::optional<HttpError> validatePaymentPayload(
        const MachinePayConfig &config, const ResourceConfig &resource, shared_ptr<PaymentPayload> paymentPayload);


    [[nodiscard]] MachinePayApp &app() const { return app_; }

    void recordSuccessfulSettlement(const PaymentPayload &payload, const EIP712Domain &domain,
                                    const ResourceConfig &resource, const OrganizationConfig &organization,
                                    const Hash &transactionHash,
                                    const string &ipAddress);

    std::optional<HttpError> checkAgainstAlreadySettledPayments(const ptr<PaymentPayload> &paymentPayload,
                                                                const ptr<EIP712Domain> &domain);

    bool lockPaymentAsBeingSettled(ptr<Authorization> authorization, ptr<EIP712Domain> domain);

    void unlockPaymentAsBeingSettled(ptr<Authorization> _authorization, ptr<EIP712Domain> _domain);

    variant<SettlementResponse, HttpError> checkPaymentIsNewAndSettleItUnsafe(const NetworkConfig &networkConfig,
                                                                              const ResourceConfig &resource,
                                                                              const OrganizationConfig &organization,
                                                                              shared_ptr<PaymentPayload> paymentPayload,
                                                                              const string &ipAddress);

    variant<SettlementResponse, HttpError> checkPaymentIsNewAndSettleIt(
        const NetworkConfig &networkConfig,
        const ResourceConfig &resource,
        const OrganizationConfig &organization,
        shared_ptr<PaymentPayload> paymentPayload,
        const string &ipAddress);


    variant<SettlementResponse, HttpError> decodeValidateAndSettlePayment(
        const std::unique_ptr<proxygen::HTTPMessage> &req,
        const MachinePayConfig &config,
        const ResourceConfig &resource,
        const OrganizationConfig &organization,
        ptr<Authorization> &outAuthorization);

private:
    MachinePayApp &app_;

    std::set<std::string> currentlySettlingPayments_;
    std::mutex currentlySettlingPaymentsMutex_;
};
