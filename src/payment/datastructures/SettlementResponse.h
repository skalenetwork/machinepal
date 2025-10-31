#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SettlementResponse {
public:
    // Explicitly default copy constructor and assignment
    SettlementResponse(const SettlementResponse&) = default;
    SettlementResponse& operator=(const SettlementResponse&) = default;
    // Getters
    bool success() const;
    const std::optional<std::string> &errorReason() const;
    const std::string &transaction() const;
    const std::string &network() const;
    const std::string &payer() const;
    // Equality operator
    bool operator==(const SettlementResponse &other) const;
    // JSON serialization
    json toJson() const;
    // JSON deserialization
    static SettlementResponse fromJsonString(const std::string &jsonString);
    // Return base64 encoding of original JSON string captured at construction
    std::string originalJsonToBase64() const;

private:
    bool success_;
    std::optional<std::string> errorReason_;
    std::string transaction_;
    std::string network_;
    std::string payer_;
    std::string originalJson_;

    SettlementResponse(bool success,
                      std::optional<std::string> errorReason,
                      const std::string &transaction,
                      const std::string &network,
                      const std::string &payer, const std::string& originalJson);

};