#pragma once

#include "MachinePayCommon.h"
#include "crypto/EthAddress.h"
#include <string>
#include <optional>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>
#include "config/JsonUtils.h"


class ResourceConfig;
class HttpError;
class MachinePayConfig; // forward declaration added
using json = nlohmann::json;

class Authorization {
public:
    Authorization();
    Authorization(const std::string& fromStr,
                  const std::string& toStr,
                  const std::string& value,
                  const std::string& validAfter,
                  const std::string& validBefore,
                  const std::string& nonce);

    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const std::string& validAfter() const;
    [[nodiscard]] const std::string& validBefore() const;
    [[nodiscard]] const std::string& nonce() const;


    // Raw address bytes
    [[nodiscard]] EthAddress from() const { return from_; }
    [[nodiscard]] EthAddress to() const { return to_; }

    bool operator==(const Authorization& other) const;

    // JSON serialization
    static std::shared_ptr<Authorization> fromJson(const json& j);
    [[nodiscard]] json toJson() const;


    std::optional<HttpError> validate(const MachinePayConfig& config, const ResourceConfig& resource);
private:


    EthAddress from_{};
    EthAddress to_{};
    std::string fromHex_;
    std::string toHex_;
    std::string value_;
    std::string validAfter_;
    std::string validBefore_;
    std::string nonce_;
};
