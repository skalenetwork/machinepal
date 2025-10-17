#pragma once
#include "common.h"
#include "OrganizationConfig.h"
#include "ServerConfig.h"
#include "ResourceConfig.h"


#include <nlohmann/json.hpp>
#include <string>
#include <memory>

class OrganizationConfig {
    ptr<vector<ptr<ResourceConfig>>> server_;
    std::string organizationName_;
    std::string subdomain_;
public:
    OrganizationConfig(const ptr<vector<ptr<ResourceConfig>>> & server, const std::string& organizationName)
        : server_(server), organizationName_(organizationName) {}
    const ptr<vector<ptr<ResourceConfig>>> & server() const { return server_; }
    const std::string& organizationName() const { return organizationName_; }
    static ptr<OrganizationConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};

