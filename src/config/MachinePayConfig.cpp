//
// Created by kladko on 9/29/25.
//

#include "common.h"

#include "MachinePayConfig.h"
#include "ConfigLoader.h"
#include "ServerConfig.h"
#include "FacilitatorConfig.h"
#include "NetworkConfig.h"
#include "LogConfig.h"
#include "OrganizationConfig.h"
#include "filesystem/FileManager.h"

#include <nlohmann/json.hpp>
#include <filesystem>


ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    CHECK_STATE(j.is_object());
    return ptr<LogConfig>( new LogConfig( ConfigLoader::getStringWithDefault(j, "level", "info"),
        ConfigLoader::getStringWithDefault(j, "type", "plain")));
}


ptr<FacilitatorConfig> FacilitatorConfig::createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{

    CHECK_STATE(fileManager);
    try
    {
        CHECK_STATE(j.is_object());
        std::optional<CanonicalPath> apiKeyFile = std::nullopt;
        auto userProvidedApiKeyFile = ConfigLoader::getStringWithDefault(j, "api_key_file", "");
        if (!userProvidedApiKeyFile.empty())
        {
            auto resolved = fileManager->checkFileExistsAndReadableAndResolve(userProvidedApiKeyFile);
            apiKeyFile = CanonicalPath(resolved);
        }
        return std::make_shared<FacilitatorConfig>(
            ConfigLoader::getStringWithDefault(j, "type", ""),
            ConfigLoader::getStringWithDefault(j, "base_url", ""),
            apiKeyFile
        );
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED;
    }
}


ptr<MachinePayConfig> MachinePayConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    try
    {
        CHECK_STATE(fileManager);
        CHECK_STATE2(j.count("server") != 0, "Missing required 'server' config section");
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

        auto resources = ResourceConfig::createVectorFromJsonArray(j);
        auto defaultOrganization = std::make_shared<OrganizationConfig>(resources, "");
        auto organizations = std::make_shared<std::vector<ptr<OrganizationConfig>>>();
        organizations->push_back(defaultOrganization);

        auto networkConfig = NetworkConfig::createFromJson(j, fileManager);

        return std::make_shared<MachinePayConfig>(serverConfig, logConfig, organizations, networkConfig);
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED;
    }
}


ptr<ServerConfig> ServerConfig::createFromJson(const nlohmann::json& j,  ptr<FileManager> fileManager)
{

    try
    {
        CHECK_STATE(fileManager);
        CHECK_STATE(j.is_object());


        ptr<HTTPConfig> httpConfig = nullptr;

        if (j.count("http") > 0)
        {
            const auto& jt = j.at("http");
            httpConfig = HTTPConfig::createFromJson(jt, fileManager);
        }
        ptr<HTTPSConfig> httpsConfig = nullptr;
        if (j.count("https") > 0)
        {
            CHECK_STATE(j.at("https").is_object());
            const auto& jt = j.at("https");
            httpsConfig = HTTPSConfig::createFromJson(jt, fileManager);

        }
        if (!httpConfig && !httpsConfig)
        {
            throw std::runtime_error("At least one of HTTP or HTTPS must be configured in server config");
        }

        return std::make_shared<ServerConfig>(
            ConfigLoader::getStringWithDefault(j, "hostname", ""),
            ConfigLoader::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
            httpConfig,
            httpsConfig
        );
    } catch (exception& ex)
    {
        RETHROW_NESTED;
    }
}
#include <stdexcept>

MachinePayConfig::MachinePayConfig(const ptr<ServerConfig>& server,
                                   const ptr<LogConfig>& log,
                                   const ptr<std::vector<ptr<OrganizationConfig>> >& organizations,
                                   std::shared_ptr<NetworkConfig> network)
    : server_(server), log_(log), network_(network) {
    CHECK_STATE(server);
    CHECK_STATE(log_);
    organizations_ = std::make_shared<std::map<string,ptr<OrganizationConfig>>>();
    for (const auto& org : *organizations) {
        CHECK_STATE(org);
        auto orgName = org->organizationName();
        CHECK_STATE2(!organizations_->contains(orgName),
            "Duplicate organization name in config: " + orgName);
        organizations_->emplace(orgName,  org);
    }
    CHECK_STATE(organizations_->contains("")); // Default organization must be present
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
