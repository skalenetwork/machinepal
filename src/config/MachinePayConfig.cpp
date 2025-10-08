//
// Created by kladko on 9/29/25.
//

#include "MachinePayConfig.h"
#include "MachinePayConfigLoader.h"
#include <filesystem>


ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json& j)
{
    CHECK_STATE(j.is_object());
    return std::make_shared<LogConfig>(
        MachinePayConfigLoader::getStringWithDefault(j, "level", "info"),
        MachinePayConfigLoader::getStringWithDefault(j, "type", "plain"));
}

static void checkFileExistsAndReadable(const std::string& path)
{
    if (path.empty())
    {
        throw std::runtime_error("File path is empty");
    }
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("File '" + path + "' does not exist");
    }

    if (!std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error("File '" + path + "' is not a regular file (a directory?)");
    }
    if (access(path.c_str(), R_OK) != 0)
    {
        throw std::runtime_error("File '" + path + "' is not readable");
    }
    if (std::filesystem::file_size(path) == 0)
    {
        throw std::runtime_error("File '" + path + "' is empty");
    }
}

ptr<FacilitatorConfig> FacilitatorConfig::createFomJson(const nlohmann::json& j)
{
    try
    {
        CHECK_STATE(j.is_object());
        std::optional<std::string> apiKeyFile = std::nullopt;
        if (j.contains("api_key_file") && !j.at("api_key_file").is_null())
        {
            std::string file = j.at("api_key_file").get<std::string>();
            checkFileExistsAndReadable(file);
            apiKeyFile = file;
        }
        return std::make_shared<FacilitatorConfig>(
            MachinePayConfigLoader::getStringWithDefault(j, "type", ""),
            MachinePayConfigLoader::getStringWithDefault(j, "base_url", ""),
            apiKeyFile
        );
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED("FacilitatorConfig::createFomJson failed: ");
    }
}


ptr<MachinePayConfig> MachinePayConfig::createFromJson(const nlohmann::json& j)
{
    try
    {
        CHECK_STATE2(j.count("server") != 0, "Missing required 'server' config section");
        auto serverConfig = ServerConfig::createFromJson(j.at("server"));

        CHECK_STATE2(j.count("facilitator") != 0, "Missing required 'facilitator' config section");
        auto facilitatorConfig = FacilitatorConfig::createFomJson(j.at("facilitator"));

        ptr<LogConfig> logConfig;
        if (j.count("log") == 0)
        {
            // Default log config if not log element is present
            logConfig = LogConfig::createDefault();
        }
        else
        {
            logConfig = LogConfig::createFromJson(j.at("log"));
        }

        return std::make_shared<MachinePayConfig>(serverConfig, facilitatorConfig, logConfig);
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED("MachinePayConfig::createFromJson failed: ");
    }
}


ptr<ServerConfig> ServerConfig::createFromJson(const nlohmann::json& j)
{
    try
    {
        CHECK_STATE(j.is_object());
        ptr<HTTPConfig> httpConfig = nullptr;
        if (j.count("http") > 0)
        {
            CHECK_STATE(j.at("http").is_object());
            const auto& jt = j.at("http");
            httpConfig = make_shared<HTTPConfig>(
                MachinePayConfigLoader::getBoolWithDefault(jt, "enabled", true),
                MachinePayConfigLoader::getUint16WithDefault(jt, "port", 8080)
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
                checkFileExistsAndReadable(file);
                caFile = file;
            }
            std::optional<std::string> keyPassFile = std::nullopt;
            if (jt.contains("key_pass_file") && !jt.at("key_pass_file").is_null())
            {
                std::string file = jt.at("key_pass_file").get<std::string>();
                checkFileExistsAndReadable(file);
                keyPassFile = file;
            }
            httpsConfig = make_shared<HTTPSConfig>(
                MachinePayConfigLoader::getBoolWithDefault(jt, "enabled", true),
                MachinePayConfigLoader::getUint16WithDefault(jt, "port", 8080),
                MachinePayConfigLoader::getStringWithDefault(jt, "cert_file", ""),
                MachinePayConfigLoader::getStringWithDefault(jt, "key_file", ""),
                keyPassFile,
                caFile);
        }
        if (!httpConfig && !httpsConfig)
        {
            throw std::runtime_error("At least one of HTTP or HTTPS must be configured in server config");
        }

        return std::make_shared<ServerConfig>(
            MachinePayConfigLoader::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
            httpConfig,
            httpsConfig
        );
    } catch (exception& ex)
    {
        RETHROW_NESTED("ServerConfig::createFromJson failed: ");
    }
}
