#pragma once
#include "PaymentPayload.h"
#include "nlohmann/json.hpp"
#include <string>
#include <ostream>
using json = nlohmann::json;

class Payload {
public:
    Payload() = default;
    Payload(const std::string& signature, const Authorization& authorization)
        : signature_(signature),
          authorization_(authorization) {}

    [[nodiscard]] const std::string& signature() const { return signature_; }
    [[nodiscard]] const Authorization& authorization() const { return authorization_; }

    bool operator==(const Payload& other) const {
        return signature_ == other.signature_ &&
               authorization_ == other.authorization_;
    }

    // JSON serialization
    friend void to_json(json& j, const Payload& p) {
        j = json{{"signature", p.signature_}, {"authorization", p.authorization_}};
    }
    friend void from_json(const json& j, Payload& p) {
        p = Payload(j.at("signature").get<std::string>(),
                    j.at("authorization").get<Authorization>());
    }

private:
    std::string signature_;
    Authorization authorization_;
};
