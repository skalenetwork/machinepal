#pragma once

#include "common.h"

class FileManager;

class HTTPConfig {
public:
    bool isEnabled() const;
    uint16_t port() const;
    static ptr<HTTPConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);

protected:
    HTTPConfig(bool isEnabled, uint16_t port);
    bool isEnabled_;
    uint16_t  port_;
};
