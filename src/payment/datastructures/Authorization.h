#pragma once

#include "MachinePayCommon.h"
#include "crypto/EthAddress.h"
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
    Authorization(const EthAddress &from, const EthAddress &to, const EIP3009Value &value,
        const EIP3009ValidityTime &valid_after, const EIP3009ValidityTime &valid_before, const EIP3009Nonce &nonce)
        : from_(from),
          to_(to),
          value_(value),
          validAfter_(valid_after),
          validBefore_(valid_before),
          nonce_(nonce) {
    }

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

    std::optional<HttpError> checkValidityTime();


    std::optional<HttpError> validate(const MachinePayConfig& config, const ResourceConfig& resource);
private:


    EthAddress from_{};
    EthAddress to_{};
    EIP3009Value value_;
    EIP3009ValidityTime validAfter_;
    EIP3009ValidityTime validBefore_;
    EIP3009Nonce nonce_;
};
