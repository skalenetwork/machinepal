#include "Payload.h"
#include <stdexcept>

// Payload implementations
Payload::Payload() = default;

Payload::Payload(const std::string& signature, const Authorization& authorization)
    : signature_(signature), authorization_(authorization) {}

const std::string& Payload::signature() const {
    return signature_;
}

const Authorization& Payload::authorization() const {
    return authorization_;
}

bool Payload::operator==(const Payload& other) const {
    return signature_ == other.signature_ &&
           authorization_ == other.authorization_;
}

std::shared_ptr<Payload> Payload::fromJson(const json& j) {
    if (!j.contains("signature") || !j.contains("authorization")) {
        throw std::invalid_argument("Missing required field in Payload JSON");
    }
    return std::make_shared<Payload>(
        j.at("signature").get<std::string>(),
        *Authorization::fromJson(j.at("authorization"))
    );
}

json Payload::toJson() const {
    json j;
    j["signature"] = signature_;
    j["authorization"] = authorization_.toJson();
    return j;
}
