#pragma once
#include <string>
#include <regex>
#include <memory>
#include <nlohmann/json.hpp>
#include "common.h"
#include "HTTPConfig.h"
#include "HTTPSConfig.h"

class FileManager;

class ServerConfig {
    std::string bindIp_;
    ptr<HTTPConfig> http_;
    ptr<HTTPSConfig> https_;
    std::string hostName_;
public:
    ServerConfig(const std::string& bindIp,
                 ptr<HTTPConfig> http,
                 ptr<HTTPSConfig> https);
    const std::string& bindIp() const;
    const ptr<HTTPConfig> http() const;
    const ptr<HTTPSConfig> https() const;
    static ptr<ServerConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};

