#include "PaymentPayload.h"
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

// PaymentPayload implementations
PaymentPayload::PaymentPayload() = default;

PaymentPayload::PaymentPayload(int x402Version, const std::string& scheme, const std::string& network, const Payload& payload)
    : x402Version_(x402Version), scheme_(scheme), network_(network), payload_(payload) {}

int PaymentPayload::x402Version() const {
    return x402Version_;
}

const std::string& PaymentPayload::scheme() const {
    return scheme_;
}

const std::string& PaymentPayload::network() const {
    return network_;
}

const Payload& PaymentPayload::payload() const {
    return payload_;
}

bool PaymentPayload::operator==(const PaymentPayload& other) const {
    return x402Version_ == other.x402Version_ &&
           scheme_ == other.scheme_ &&
           network_ == other.network_ &&
           payload_ == other.payload_;
}

std::shared_ptr<PaymentPayload> PaymentPayload::fromJson(const json& j) {
    if (!j.contains("x402Version") || !j.contains("scheme") || !j.contains("network") || !j.contains("payload")) {
        throw std::invalid_argument("Missing required field in PaymentPayload JSON");
    }
    return std::make_shared<PaymentPayload>(
        j.at("x402Version").get<int>(),
        j.at("scheme").get<std::string>(),
        j.at("network").get<std::string>(),
        *Payload::fromJson(j.at("payload"))
    );
}

json PaymentPayload::toJson() const {
    json j;
    j["x402Version"] = x402Version_;
    j["scheme"] = scheme_;
    j["network"] = network_;
    j["payload"] = payload_.toJson();
    return j;
}
