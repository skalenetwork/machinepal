#include "Authorization.h"

Authorization::Authorization() = default;

Authorization::Authorization(const std::string& from,
                             const std::string& to,
                             const std::string& value,
                             const std::string& validAfter,
                             const std::string& validBefore,
                             const std::string& nonce)
    : from_(from),
      to_(to),
      value_(value),
      validAfter_(validAfter),
      validBefore_(validBefore),
      nonce_(nonce) {}

const std::string& Authorization::from() const { return from_; }
const std::string& Authorization::to() const { return to_; }
const std::string& Authorization::value() const { return value_; }
const std::string& Authorization::validAfter() const { return validAfter_; }
const std::string& Authorization::validBefore() const { return validBefore_; }
const std::string& Authorization::nonce() const { return nonce_; }

bool Authorization::operator==(const Authorization& other) const {
    return from_ == other.from_ &&
           to_ == other.to_ &&
           value_ == other.value_ &&
           validAfter_ == other.validAfter_ &&
           validBefore_ == other.validBefore_ &&
           nonce_ == other.nonce_;
}

std::shared_ptr<Authorization> Authorization::fromJson(const json& j) {
    CHECK_STATE_JSON(j.contains("from"), "Missing required field 'from' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("to"), "Missing required field 'to' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("value"), "Missing required field 'value' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("validAfter"), "Missing required field 'validAfter' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("validBefore"), "Missing required field 'validBefore' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("nonce"), "Missing required field 'nonce' in Authorization JSON", j);

    CHECK_STATE_JSON(j.at("from").is_string(), "'from' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("to").is_string(), "'to' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("value").is_string(), "'value' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("validAfter").is_string(), "'validAfter' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("validBefore").is_string(), "'validBefore' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("nonce").is_string(), "'nonce' must be a string in Authorization JSON", j);

    return std::make_shared<Authorization>(
        j.at("from").get<std::string>(),
        j.at("to").get<std::string>(),
        j.at("value").get<std::string>(),
        j.at("validAfter").get<std::string>(),
        j.at("validBefore").get<std::string>(),
        j.at("nonce").get<std::string>()
    );
}

json Authorization::toJson() const {
    json j;
    j["from"] = from_;
    j["to"] = to_;
    j["value"] = value_;
    j["validAfter"] = validAfter_;
    j["validBefore"] = validBefore_;
    j["nonce"] = nonce_;
    return j;
}

