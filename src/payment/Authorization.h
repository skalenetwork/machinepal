#pragma once

#include <string>
#include <optional>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>
#include "config/JsonUtils.h"

using json = nlohmann::json;

class Authorization {
public:
    Authorization();
    Authorization(const std::string& from,
                  const std::string& to,
                  const std::string& value,
                  const std::string& validAfter,
                  const std::string& validBefore,
                  const std::string& nonce);

    // Getters
    [[nodiscard]] const std::string& from() const;
    [[nodiscard]] const std::string& to() const;
    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const std::string& validAfter() const;
    [[nodiscard]] const std::string& validBefore() const;
    [[nodiscard]] const std::string& nonce() const;

    bool operator==(const Authorization& other) const;

    // JSON serialization
    static std::shared_ptr<Authorization> fromJson(const json& j);
    [[nodiscard]] json toJson() const;

private:
    std::string from_;
    std::string to_;
    std::string value_;
    std::string validAfter_;
    std::string validBefore_;
    std::string nonce_;
};
