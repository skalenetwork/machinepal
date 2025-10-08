//
// Created by kladko on 9/29/25.
//

#include "MachinePayConfig.h"
#include "MachinePayConfigLoader.h"


ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json &j) {
    CHECK_STATE(j.is_object());
    return std::make_shared<LogConfig>(
        MachinePayConfigLoader::getStringWithDefault(j, "level", "info"),
        MachinePayConfigLoader::getStringWithDefault(j, "type", "plain"));
}

ptr<FacilitatorConfig> FacilitatorConfig::createFomJson(const nlohmann::json &j) {
    CHECK_STATE(j.is_object());
    return std::make_shared<FacilitatorConfig>(
        MachinePayConfigLoader::getStringWithDefault(j, "type", ""),
        MachinePayConfigLoader::getStringWithDefault(j, "base_url", ""),
        (j.contains("api_key_file") && !j.at("api_key_file").is_null())
            ? std::optional<std::string>(j.at("api_key_file").get<std::string>())
            : std::nullopt
    );
}


ptr<MachinePayConfig> MachinePayConfig::createFromJson(const nlohmann::json& j) {
    CHECK_STATE2(j.count("server") != 0, "Missing required 'server' config section");
    auto serverConfig = ServerConfig::createFromJson(j.at("server"));

    CHECK_STATE2(j.count("facilitator") != 0, "Missing required 'facilitator' config section");
    auto facilitatorConfig = FacilitatorConfig::createFomJson(j.at("facilitator"));

    ptr<LogConfig> logConfig;
    if (j.count("log") == 0) {
        // Default log config if not log element is present
        logConfig = LogConfig::createDefault();
    } else {
        logConfig = LogConfig::createFromJson(j.at("log"));
    }

    return std::make_shared<MachinePayConfig>(serverConfig, facilitatorConfig, logConfig);
}



ptr<ServerConfig> ServerConfig::createFromJson(const nlohmann::json& j) {
    CHECK_STATE(j.is_object());

    ptr<HTTPConfig> httpConfig = nullptr;

    if (j.count("http") > 0) {
        CHECK_STATE(j.at("http").is_object());
        const auto &jt = j.at("http");
        httpConfig = make_shared<HTTPConfig>(
            MachinePayConfigLoader::getBoolWithDefault(jt, "enabled", true),
            MachinePayConfigLoader::getUint16WithDefault(jt, "port",8080)
        );
    }



    ptr<HTTPSConfig> httpsConfig = nullptr;


    if (j.count("https") > 0) {
        CHECK_STATE(j.at("https").is_object());
        const auto &jt = j.at("https");

        auto caFile = (jt.contains("ca_file") && !jt.at("ca_file").is_null()) ?
            std::optional<std::string>(jt.at("ca_file").get<std::string>()) : std::nullopt;


        auto keyPassFile = (jt.contains("key_pass_file") && !jt.at("key_pass_file").is_null()) ?
            std::optional<std::string>(jt.at("key_pass_file").get<std::string>()) : std::nullopt;

        httpsConfig = make_shared<HTTPSConfig>(
            MachinePayConfigLoader::getBoolWithDefault(jt, "enabled", true),
            MachinePayConfigLoader::getUint16WithDefault(jt, "port",8080),
            MachinePayConfigLoader::getStringWithDefault(jt, "cert_file", ""),
                        MachinePayConfigLoader::getStringWithDefault(jt, "key_file", ""),
                        keyPassFile,
                        caFile);
    }


    if (!httpConfig && !httpsConfig) {
        throw std::runtime_error("At least one of HTTP or HTTPS must be configured in server config");
    }


    return  std::make_shared<ServerConfig>(
        MachinePayConfigLoader::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
        httpConfig,
        httpsConfig
    );
}