//
// Created by kladko on 9/29/25.
//

#include "MachinePayCommon.h"

#include "MachinePayConfig.h"
#include "ConfigLoader.h"
#include "subconfigs/ServerConfig.h"
#include "subconfigs/FacilitatorConfig.h"
#include "subconfigs/NetworkConfig.h"
#include "subconfigs/LogConfig.h"
#include "subconfigs/OrganizationConfig.h"
#include "filesystem/FileManager.h"

#include <nlohmann/json.hpp>
#include <filesystem>

#include "JsonUtils.h"





ptr<MachinePayConfig> MachinePayConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    try
    {
        CHECK_STATE(fileManager);
        CHECK_STATE_JSON(j.count("server") != 0, "Missing required 'server' config section", j);
        auto serverConfig = ServerConfig::createFromJson(j.at("server"), fileManager);

        ptr<LogConfig> logConfig;
        if (j.count("log") == 0)
        {
            // Default log config if not log element is present
            logConfig = LogConfig::createDefault();
        }
        else
        {
            logConfig = LogConfig::createFromJson(j.at("log"), fileManager);
        }

        auto resources = ResourceConfig::createVectorFromJsonArray(j, fileManager);
        auto defaultOrganization = OrganizationConfig::createDefaultFromResources(resources);
        auto networkConfig = NetworkConfig::createFromJson(j, fileManager);
        auto organizations = OrganizationConfig::createVectorFromJsonArray(j, fileManager);
        organizations->push_back(defaultOrganization);

        return ptr<MachinePayConfig>(new MachinePayConfig(serverConfig, logConfig, organizations, networkConfig));
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED;
    }
}




MachinePayConfig::MachinePayConfig(const ptr<ServerConfig>& server,
                                   const ptr<LogConfig>& log,
                                   const ptr<std::vector<ptr<OrganizationConfig>> >& organizations,
                                   std::shared_ptr<NetworkConfig> network)
    : server_(server), log_(log), network_(network) {
    CHECK_STATE(server);
    CHECK_STATE(log_);
    organizationsByName_ = std::make_shared<std::map<string,ptr<OrganizationConfig>>>();
    organizationsBySubdomain_ = std::make_shared<std::map<string,ptr<OrganizationConfig>>>();
    for (const auto& org : *organizations) {
        CHECK_STATE(org);
        auto orgName = org->organizationName();
        CHECK_STATE2(!organizationsByName_->contains(orgName),
            "Duplicate organization name in config: " + orgName);
        organizationsByName_->emplace(orgName,  org);

        auto subdomain = org->subdomain();
        CHECK_STATE2(!organizationsBySubdomain_->contains(subdomain),
            "Duplicate organization domain in config: " + subdomain);
        organizationsBySubdomain_->emplace(subdomain,  org);


    }
    CHECK_STATE(organizationsByName_->contains("")); // Default organization must be present
}

const std::shared_ptr<NetworkConfig>& MachinePayConfig::network() const {
    CHECK_STATE(network_);
    return network_;
}

const ptr<LogConfig>& MachinePayConfig::log() const {
    CHECK_STATE(log_);
    return log_;
}
const ptr<ServerConfig>& MachinePayConfig::server() const {
    CHECK_STATE(server_);
    return server_;
}


