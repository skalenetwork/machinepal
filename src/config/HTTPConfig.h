#pragma once

#include "common.h"

class HTTPConfig {
protected:
    bool isEnabled_;
    uint16_t port_;
public:
    HTTPConfig(bool isEnabled, uint16_t port);
    bool isEnabled() const;
    uint16_t port() const;
    static ptr<HTTPConfig> createFromJson(const nlohmann::json& j);
};
