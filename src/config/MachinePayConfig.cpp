//
// Created by kladko on 9/29/25.
//

#include "MachinePayConfig.h"
#include "ConfigLoader.h"
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
        std::optional<filesystem::path> apiKeyFile = std::nullopt;
        if (j.contains("api_key_file") && !j.at("api_key_file").is_null())
        {
            std::string file = j.at("api_key_file").get<std::string>();
            apiKeyFile = fileManager->checkFileExistsAndReadableAndResolve(file);
        }
        return std::make_shared<FacilitatorConfig>(
            ConfigLoader::getStringWithDefault(j, "type", ""),
            ConfigLoader::getStringWithDefault(j, "base_url", ""),
            apiKeyFile
        );
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED("FacilitatorConfig::createFomJson failed: ");
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

        return std::make_shared<MachinePayConfig>(serverConfig, facilitatorConfig, logConfig);
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED("MachinePayConfig::createFromJson failed: ");
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
            std::optional<std::string> caFile = std::nullopt;
            if (jt.contains("ca_file") && !jt.at("ca_file").is_null())
            {
                std::string file = jt.at("ca_file").get<std::string>();
                caFile = fileManager->checkFileExistsAndReadableAndResolve(file);
            }
            std::optional<filesystem::path> keyPassFile = std::nullopt;
            if (jt.contains("key_pass_file") && !jt.at("key_pass_file").is_null())
            {
                std::string file = jt.at("key_pass_file").get<std::string>();
                keyPassFile = fileManager->checkFileExistsAndReadableAndResolve(file);
            }
            httpsConfig = make_shared<HTTPSConfig>(
                ConfigLoader::getBoolWithDefault(jt, "enabled", true),
                ConfigLoader::getUint16WithDefault(jt, "port", 8080),
                ConfigLoader::getStringWithDefault(jt, "cert_file", ""),
                ConfigLoader::getStringWithDefault(jt, "key_file", ""),
                keyPassFile,
                caFile);
        }
        if (!httpConfig && !httpsConfig)
        {
            throw std::runtime_error("At least one of HTTP or HTTPS must be configured in server config");
        }

        return std::make_shared<ServerConfig>(
            ConfigLoader::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
            httpConfig,
            httpsConfig
        );
    } catch (exception& ex)
    {
        RETHROW_NESTED("ServerConfig::createFromJson failed: ");
    }
}
