#pragma once
#include "ServerConfig.h"
#include "common.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class OrganizationConfig {
    ptr<ServerConfig> server_;
    std::string organizationName_;
public:
    OrganizationConfig(const ptr<ServerConfig>& server, const std::string& organizationName)
        : server_(server), organizationName_(organizationName) {}
    const ptr<ServerConfig>& server() const { return server_; }
    const std::string& organizationName() const { return organizationName_; }
    static ptr<OrganizationConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};

