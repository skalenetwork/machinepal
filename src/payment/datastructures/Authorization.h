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
#include "crypto/EIP3009ValidityTime.h"
#include "crypto/EIP3009Value.h"


class ResourceConfig;
class HttpError;
class MachinePayConfig; // forward declaration added
using json = nlohmann::json;

class Authorization {
public:
    Authorization(const std::string& fromStr,
                  const std::string& toStr,
                  const std::string& value,
                  const std::string& validAfter,
                  const std::string& validBefore,
                  const std::string& nonce);


    [[nodiscard]] const EIP3009Value& value() const;
    [[nodiscard]] const EIP3009ValidityTime& validAfter() const;
    [[nodiscard]] const EIP3009ValidityTime& validBefore() const;
    [[nodiscard]] const EIP3009Nonce& nonce() const;


    // Raw address bytes
    [[nodiscard]] const EthAddress& from() const { return from_; }
    [[nodiscard]] const EthAddress& to() const { return to_; }

    bool operator==(const Authorization& other) const;

    // JSON serialization
    static std::shared_ptr<Authorization> fromJson(const json& j);
    [[nodiscard]] json toJson() const;


    std::optional<HttpError> validate(const MachinePayConfig& config, const ResourceConfig& resource);
private:


    EthAddress from_{};
    EthAddress to_{};
    EIP3009Value value_;
    EIP3009ValidityTime validAfter_;
    EIP3009ValidityTime validBefore_;
    EIP3009Nonce nonce_;
};
