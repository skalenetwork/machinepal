#pragma once
#include <string>
#include <glog/types.h>
#include <optional>
#include <cstdint>
#include <regex>
#include <nlohmann/json.hpp>

class TlsConfig {
public:
    std::string certFile;             // required
    std::string keyFile;              // required
    std::string keyPassFile;          // required (but secret file path)
    std::optional<std::string> caFile; // optional (system CA used if not set)

    TlsConfig(const std::string& certFile,
              const std::string& keyFile,
              const std::string& keyPassFile,
              const std::optional<std::string>& caFile = std::nullopt)
        : certFile(certFile), keyFile(keyFile), keyPassFile(keyPassFile), caFile(caFile) {}
};

class FacilitatorConfig {
public:
    std::string type;                 // e.g., "cdp" or "x402"
    std::string baseUrl;              // required
    std::optional<std::string> apiKeyFile; // optional (secret file path)

    FacilitatorConfig(const std::string& type,
                     const std::string& baseUrl,
                     const std::optional<std::string>& apiKeyFile = std::nullopt)
        : type(type), baseUrl(baseUrl), apiKeyFile(apiKeyFile) {}
};

class ServerConfig {
public:
    bool httpEnabled; // required
    bool httpsEnabled; // required
    std::optional<uint16_t> httpPort;
    std::optional<uint16_t>  httpsPort;
    std::string bindIp; // default to INADDR_ANY
    std::optional<TlsConfig> tls;                    // nested struct for clarity

    ServerConfig(bool httpEnabled,
                 bool httpsEnabled,
                 std::optional<uint16_t> httpPort,
                 std::optional<uint16_t> httpsPort,
                 const std::string& bindIp,
                 const TlsConfig& tls)
        : httpEnabled(httpEnabled), httpsEnabled(httpsEnabled),
          httpPort(httpPort), httpsPort(httpsPort),
          bindIp(bindIp), tls(tls) {
        // Basic validation
        if (httpEnabled && !httpPort) {
            throw std::invalid_argument("HTTP is enabled but httpPort is not set");
        }

        if (httpPort == 0) {
            throw std::invalid_argument("HTTP ports is set to zero");
        }

        if (httpsEnabled && !httpsPort) {
            throw std::invalid_argument("HTTPS is enabled but httpsPort is not set");
        }

        if (httpPort == 0) {
            throw std::invalid_argument("HTTP port is set to zero");
        }

        if (bindIp.empty()) {
            throw std::invalid_argument("bindIp cannot be empty");
        }
        // Validate bindIp is a valid IPv4 or IPv6 address
        static const std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        static const std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
        if (!std::regex_match(bindIp, ipv4_regex) && !std::regex_match(bindIp, ipv6_regex) && bindIp != "0.0.0.0" && bindIp != "::") {
            throw std::invalid_argument("bindIp is not a valid IPv4 or IPv6 address");
        }
    }
};

class MachinePayConfig {
public:
    ServerConfig server;
    FacilitatorConfig facilitator;

    MachinePayConfig(const ServerConfig& server,
                     const FacilitatorConfig& facilitator)
        : server(server), facilitator(facilitator) {}

    static std::string getStringWitHDefault(const nlohmann::json &j, const std::string &key, const std::string &defaultValue);
};