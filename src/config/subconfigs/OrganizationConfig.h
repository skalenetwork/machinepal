#pragma once
#include "MachinepayCommon.h"
#include "OrganizationConfig.h"
#include "ServerConfig.h"
#include "ResourceConfig.h"


#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <proxygen/lib/http/HTTPMethod.h>

class OrganizationConfig {
    ptr<vector<ptr<ResourceConfig> > > resources_;
    std::string organizationName_;
    std::string subdomain_;
    std::string payToAddressStr_;

    OrganizationConfig(const ptr<vector<ptr<ResourceConfig> > > &server, const std::string &organizationName,
        const std::string &subdomain)
    : resources_(server), organizationName_(organizationName), subdomain_(subdomain) {
        payToAddressStr_ = "0x2222222222222222222222222222222222222222";
    }

public:
    [[nodiscard]] std::string payToAddressAsString() const {
        return payToAddressStr_;
    }


    const ptr<vector<ptr<ResourceConfig> > > &resources() const { return resources_; }
    const std::string &organizationName() const { return organizationName_; }
    const std::string &subdomain() const { return subdomain_; }

    static ptr<OrganizationConfig> createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager);

    static std::shared_ptr<std::vector<ptr<OrganizationConfig> > > createVectorFromJsonArray(
        const nlohmann::json &j, ptr<FileManager> fileManager);

    static ptr<OrganizationConfig> createDefaultFromResources(ptr<vector<ptr<ResourceConfig> > > resources);

    ptr<ResourceConfig> getResourceByPath(const std::string& path, proxygen::HTTPMethod method, const std::string& body) const {
        CHECK_STATE(resources_);
        string matchString;
        // ignore last backslash for matching
        if (path.size() > 1 && path.back() == '/') {
            matchString = path.substr(0, path.size() - 1);
        } else {
            matchString = path;
        }
        for (const auto& resource : *resources_) {
            if (resource->machinePayPath() == matchString) {
                return resource;
            }
        }
        return nullptr;
    }
};
