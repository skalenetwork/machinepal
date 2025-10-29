#pragma once

#include "Authorization.h"
#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>

class HttpError;
class ResourceConfig;
class EIP712Domain;
;

class Payload {
public:
    Payload();
    Payload(const std::string& signature, std::shared_ptr<Authorization> authorization);

    Payload(const EIP712Signature &signature, const std::shared_ptr<Authorization> &authorization)
        : signature_(signature),
          authorization_(authorization) {
    }

    [[nodiscard]] const EIP712Signature signature() const;
    [[nodiscard]] std::shared_ptr<Authorization> authorization() const;

    bool operator==(const Payload& other) const;
    static std::shared_ptr<Payload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;

    std::optional<HttpError> validate(const MachinePayConfig& config, const ResourceConfig& resource);
    std::optional<HttpError> verifyEIP3009Signature(std::shared_ptr<EIP712Domain> domain) const;

private:
    EIP712Signature signature_;
    std::shared_ptr<Authorization> authorization_;
};
