#pragma once

#include "MachinePayCommon.h"
#include "crypto/EthAddress.h"
#include <string>
#include <optional>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>
#include "config/JsonUtils.h"
#include "crypto/EIP3009Authorization.h"


class ResourceConfig;
class HttpError;
class MachinePayConfig; // forward declaration added
using json = nlohmann::json;

class Authorization {

    Authorization(const std::string& fromStr,
                  const std::string& toStr,
                  const std::string& value,
                  const std::string& validAfter,
                  const std::string& validBefore,
                  const std::string& nonce);
    Authorization();

public:



    [[nodiscard]] const u256& value() const;
    [[nodiscard]] const u256& validAfter() const;
    [[nodiscard]] const u256& validBefore() const;
    [[nodiscard]] const EIP3009Nonce& nonce() const;


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
    u256 value_;
    u256 validAfter_;
    u256 validBefore_;
    EIP3009Nonce nonce_;
};
