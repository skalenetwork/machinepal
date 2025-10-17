//
// Created by kladko on 9/29/25.
//

#include "MachinePayConfig.h"
#include "ConfigLoader.h"
#include "ServerConfig.h"
#include "FacilitatorConfig.h"
#include "LogConfig.h"
#include "OrganizationConfig.h"
#include "common.h"
#include <nlohmann/json.hpp>
#include <filesystem>


ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    CHECK_STATE(j.is_object());
    return std::make_shared<LogConfig>(
        ConfigLoader::getStringWithDefault(j, "level", "info"),
        ConfigLoader::getStringWithDefault(j, "type", "plain"));
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
        CHECK_STATE2(j.count("server") != 0, "Missing required 'server' config section");
        auto serverConfig = ServerConfig::createFromJson(j.at("server"), fileManager);

        CHECK_STATE2(j.count("facilitator") != 0, "Missing required 'facilitator' config section");
        auto facilitatorConfig = FacilitatorConfig::createFomJson(j.at("facilitator"), fileManager);

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

        auto resources = ResourceConfig::createResourcesFromJsonArray(j);



        auto defaultOrganization = std::make_shared<OrganizationConfig>(resources, "");
        auto organizations = std::make_shared<std::vector<ptr<OrganizationConfig>>>();
        organizations->push_back(defaultOrganization);


        return std::make_shared<MachinePayConfig>(serverConfig, facilitatorConfig, logConfig, organizations);
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
            CHECK_STATE(j.at("http").is_object());
            const auto& jt = j.at("http");
            httpConfig = make_shared<HTTPConfig>(
                ConfigLoader::getBoolWithDefault(jt, "enabled", true),
                ConfigLoader::getUint16WithDefault(jt, "port", 8080)
            );
        }
        ptr<HTTPSConfig> httpsConfig = nullptr;
        if (j.count("https") > 0)
        {
            CHECK_STATE(j.at("https").is_object());
            const auto& jt = j.at("https");
            std::optional<CanonicalPath> caFile = std::nullopt;
            if (jt.contains("ca_file") && !jt.at("ca_file").is_null())
            {
                std::string file = jt.at("ca_file").get<std::string>();
                auto resolved = fileManager->checkFileExistsAndReadableAndResolve(file);
                caFile = CanonicalPath(resolved);
            }
            std::optional<CanonicalPath> keyPassFile = std::nullopt;
            if (jt.contains("key_pass_file") && !jt.at("key_pass_file").is_null())
            {
                std::string file = jt.at("key_pass_file").get<std::string>();
                auto resolved = fileManager->checkFileExistsAndReadableAndResolve(file);
                keyPassFile = CanonicalPath(resolved);
            }
            auto certFile = CanonicalPath(fileManager->checkFileExistsAndReadableAndResolve(ConfigLoader::getStringWithDefault(jt, "cert_file", "")));
            auto keyFile = CanonicalPath(fileManager->checkFileExistsAndReadableAndResolve(ConfigLoader::getStringWithDefault(jt, "key_file", "")));
            httpsConfig = make_shared<HTTPSConfig>(
                ConfigLoader::getBoolWithDefault(jt, "enabled", true),
                ConfigLoader::getUint16WithDefault(jt, "port", 8080),
                certFile,
                keyFile,
                keyPassFile,
                caFile);
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
                                   const ptr<FacilitatorConfig>& facilitator,
                                   const ptr<LogConfig>& log,
                                   const ptr<std::vector<ptr<OrganizationConfig>> >& organizations)
    : server_(server),facilitator_(facilitator), log_(log) {
    CHECK_STATE(server);
    CHECK_STATE(facilitator_);
    CHECK_STATE(log_);
    organizations_ = std::make_shared<std::map<string,ptr<OrganizationConfig>>>();
    for (const auto& org : *organizations) {
        CHECK_STATE(org);
        auto orgName = org->organizationName();
        CHECK_STATE2(!organizations_->contains(orgName),
            "Duplicate organization name in config: " + orgName);
        organizations_->emplace(orgName,  org);
    }

    CHECK_STATE(organizations_->contains("")); // Default organization must be present=
}

const ptr<FacilitatorConfig>& MachinePayConfig::facilitator() const {
    CHECK_STATE(facilitator_);
    return facilitator_;
}
const ptr<LogConfig>& MachinePayConfig::log() const {
    CHECK_STATE(log_);
    return log_;
}
const ptr<ServerConfig>& MachinePayConfig::server() const {
    CHECK_STATE(server_);
    return server_;
}
