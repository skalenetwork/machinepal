#pragma once

#include "Authorization.h"
#include <string>
#include "nlohmann/json.hpp"
#include <ostream>

using json = nlohmann::json;

class Payload {
public:
    Payload();
    Payload(const std::string& signature, const Authorization& authorization);

    [[nodiscard]] const std::string& signature() const;
    [[nodiscard]] const Authorization& authorization() const;

    bool operator==(const Payload& other) const;
    static std::shared_ptr<Payload> fromJson(const json& j);
    [[nodiscard]] json toJson() const;
private:
    std::string signature_;
    Authorization authorization_;
};
