#pragma once

#include <string>
#include <optional>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>
#include "config/JsonUtils.h"
#include "MachinePayCommon.h"

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

    // Hex string accessors (tests expect these)
    [[nodiscard]] std::string fromAsStr();  // returns 0x-prefixed lowercase hex
    [[nodiscard]] std::string toAsStr();   // returns 0x-prefixed lowercase hex

    // Raw address bytes
    [[nodiscard]] const Address& from() const { return from_; }
    [[nodiscard]] const Address& to() const { return to_; }

    bool operator==(const Authorization& other) const;

    // JSON serialization
    static std::shared_ptr<Authorization> fromJson(const json& j);
    [[nodiscard]] json toJson() const;


    std::optional<HttpError> validate(const MachinePayConfig& config, const ResourceConfig& resource);
private:
    static Address parseHexAddress(const std::string& hex);
    static std::string addressToHex(const Address& addr);

    Address from_{};
    Address to_{};
    std::string fromHex_;
    std::string toHex_;
    std::string value_;
    std::string validAfter_;
    std::string validBefore_;
    std::string nonce_;
};
