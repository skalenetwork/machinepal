#pragma once

#include "Payload.h"
#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>
#include <optional>
#include "x402_protocol/HttpError.h"
#include "config/MachinePayConfig.h"
#include "config/subconfigs/ResourceConfig.h"


class PaymentPayload {
public:
    PaymentPayload();
    PaymentPayload(int x402Version, const std::string& scheme, const std::string& network, std::shared_ptr<Payload> payload);

    [[nodiscard]] int x402Version() const;
    [[nodiscard]] const std::string& scheme() const;
    [[nodiscard]] const std::string& network() const;
    [[nodiscard]] std::shared_ptr<Payload> payload() const; // returns shared_ptr to payload

    bool operator==(const PaymentPayload& other) const;
    static std::shared_ptr<PaymentPayload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;
    std::optional<HttpError> validateAndVerifySignature(const MachinePayConfig& config, const ResourceConfig& resource) const;

    std::optional<HttpError> verifyEIP3009Signature(const MachinePayConfig &config, const ResourceConfig &resource) const;

    static ptr<PaymentPayload> createDefaultPaymentPayload(EthPrivateKey &privKey, EthAddress &to, EIP3009Value &value,
                                                    EIP3009Nonce &nonce, std::string networkName);

    std::string createHttpHeaderValue();

private:
    int x402Version_;
    std::string scheme_;
    std::string network_;
    std::shared_ptr<Payload> payload_;
};
