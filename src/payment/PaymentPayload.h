#pragma once

#include "Authorization.h"
#include <string>
#include "nlohmann/json.hpp"
#include <ostream>

using json = nlohmann::json;

class Payload {
public:
    Payload();
    Payload(const std::string& signature, const Authorization& authorization);

    [[nodiscard]] const std::string& signature() const;
    [[nodiscard]] const Authorization& authorization() const;

    bool operator==(const Payload& other) const;
    static std::shared_ptr<Payload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;
private:
    std::string signature_;
    Authorization authorization_;
};

class PaymentPayload {
public:
    PaymentPayload();
    PaymentPayload(int x402Version, const std::string& scheme, const std::string& network, const Payload& payload);

    [[nodiscard]] int x402Version() const;
    [[nodiscard]] const std::string& scheme() const;
    [[nodiscard]] const std::string& network() const;
    [[nodiscard]] const Payload& payload() const;

    bool operator==(const PaymentPayload& other) const;
    static std::shared_ptr<PaymentPayload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;
private:
    int x402Version_;
    std::string scheme_;
    std::string network_;
    Payload payload_;
};
