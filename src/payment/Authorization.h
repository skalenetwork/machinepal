#pragma once


#include <string>
#include <utility>
#include <optional>
#include "nlohmann/json.hpp"
#include <ostream>

using json = nlohmann::json;

class Authorization {
public:
    Authorization() = default;
    Authorization(const std::string& from,
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

    // Getters
    [[nodiscard]] const std::string& from() const { return from_; }
    [[nodiscard]] const std::string& to() const { return to_; }
    [[nodiscard]] const std::string& value() const { return value_; }
    [[nodiscard]] const std::string& validAfter() const { return validAfter_; }
    [[nodiscard]] const std::string& validBefore() const { return validBefore_; }
    [[nodiscard]] const std::string& nonce() const { return nonce_; }

    bool operator==(const Authorization& other) const {
        return from_ == other.from_ &&
               to_ == other.to_ &&
               value_ == other.value_ &&
               validAfter_ == other.validAfter_ &&
               validBefore_ == other.validBefore_ &&
               nonce_ == other.nonce_;
    }

    // JSON serialization
    static std::shared_ptr<Authorization> fromJson(const json& j) {
        if (!j.contains("from") || !j.contains("to") || !j.contains("value") ||
            !j.contains("validAfter") || !j.contains("validBefore") || !j.contains("nonce")) {
            throw std::invalid_argument("Missing required field in Authorization JSON");
        }
        return std::make_shared<Authorization>(
            j.at("from").get<std::string>(),
            j.at("to").get<std::string>(),
            j.at("value").get<std::string>(),
            j.at("validAfter").get<std::string>(),
            j.at("validBefore").get<std::string>(),
            j.at("nonce").get<std::string>()
        );
    }
    json toJson() const {
        json j;
        j["from"] = from_;
        j["to"] = to_;
        j["value"] = value_;
        j["validAfter"] = validAfter_;
        j["validBefore"] = validBefore_;
        j["nonce"] = nonce_;
        return j;
    }

private:
    std::string from_;
    std::string to_;
    std::string value_;
    std::string validAfter_;
    std::string validBefore_;
    std::string nonce_;
};
