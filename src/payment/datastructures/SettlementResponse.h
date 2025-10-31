#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SettlementResponse {
public:
    SettlementResponse(bool success,
                      std::optional<std::string> errorReason,
                      const std::string &transaction,
                      const std::string &network,
                      const std::string &payer);

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
    static SettlementResponse fromJson(const json &j);

private:
    bool success_;
    std::optional<std::string> errorReason_;
    std::string transaction_;
    std::string network_;
    std::string payer_;
};