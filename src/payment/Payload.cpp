#include "Payload.h"
#include <stdexcept>

#include "config/JsonUtils.h"

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
    try {
        CHECK_STATE_JSON(j.contains("signature"), "Missing required field 'signature' in Payload JSON", j);
        CHECK_STATE_JSON(j.contains("authorization"), "Missing required field 'authorization' in Payload JSON", j);

        CHECK_STATE_JSON(j.at("signature").is_string(), "'signature' must be a string in Payload JSON", j);
        CHECK_STATE_JSON(j.at("authorization").is_object(), "'authorization' must be an object in Payload JSON", j);



        return std::make_shared<Payload>(
            j.at("signature").get<std::string>(),
            *Authorization::fromJson(j.at("authorization"))
        );
    } catch (std::exception& e) {
        RETHROW_NESTED;
    }
}

json Payload::toJson() const {
    json j;
    j["signature"] = signature_;
    j["authorization"] = authorization_.toJson();
    return j;
}
