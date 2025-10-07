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

    ptr<TlsConfig> tlsConfig = nullptr;

    if (j.count("tls") > 0) {
        CHECK_STATE(j.at("tls").is_object());
        const auto &jt = j.at("tls");
        tlsConfig = make_shared<TlsConfig>(
            MachinePayConfigLoader::getStringWithDefault(jt, "cert_file", ""),
            MachinePayConfigLoader::getStringWithDefault(jt, "key_file", ""),
            MachinePayConfigLoader::getStringWithDefault(jt, "key_pass_file", ""),
            (jt.contains("ca_file") && !jt.at("ca_file").is_null()) ? std::optional<std::string>(jt.at("ca_file").get<std::string>()) : std::nullopt
        );
    }

    return  std::make_shared<ServerConfig>(
        j.at("enable_http").get<bool>(),
        j.at("enable_https").get<bool>(),
        j.at("http_listen_port").get<uint16_t>(),
        j.at("https_listen_port").get<uint16_t>(),
        MachinePayConfigLoader::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
        tlsConfig
    );
}