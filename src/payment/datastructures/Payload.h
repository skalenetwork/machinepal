#pragma once

#include "Authorization.h"
#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include <ostream>

using json = nlohmann::json;

class Payload {
public:
    Payload();
    Payload(const std::string& signature, std::shared_ptr<Authorization> authorization);

    [[nodiscard]] const std::string& signature() const;
    [[nodiscard]] std::shared_ptr<Authorization> authorization() const; // now returns shared_ptr
    [[nodiscard]] std::shared_ptr<Authorization> authorizationPtr() const; // kept for compatibility

    bool operator==(const Payload& other) const;
    static std::shared_ptr<Payload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;
private:
    std::string signature_;
    std::shared_ptr<Authorization> authorization_;
};
