#pragma once
#include "common.h"
#include "OrganizationConfig.h"
#include "ServerConfig.h"
#include "ResourceConfig.h"


#include <nlohmann/json.hpp>
#include <string>
#include <memory>

class OrganizationConfig {
    ptr<vector<ptr<ResourceConfig> > > resources_;
    std::string organizationName_;
    std::string subdomain_;

    OrganizationConfig(const ptr<vector<ptr<ResourceConfig> > > &server, const std::string &organizationName)
    : resources_(server), organizationName_(organizationName) {
    }

public:


    const ptr<vector<ptr<ResourceConfig> > > &resources() const { return resources_; }
    const std::string &organizationName() const { return organizationName_; }

    static ptr<OrganizationConfig> createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager);

    static std::shared_ptr<std::vector<ptr<OrganizationConfig> > > createVectorFromJsonArray(
        const nlohmann::json &j, ptr<FileManager> fileManager);

    static ptr<OrganizationConfig> createDefaultFromResources(ptr<vector<ptr<ResourceConfig> > > resources);
};
