#pragma once
#include <string>
#include <glog/types.h>
#include <optional>
#include <cstdint>
#include <regex>
#include <nlohmann/json.hpp>

#include "common.h"

class TlsConfig {
    std::string certFile_;
    std::string keyFile_;
    std::string keyPassFile_;
    std::optional<std::string> caFile_;
public:
    TlsConfig(const std::string& certFile,
              const std::string& keyFile,
              const std::string& keyPassFile,
              const std::optional<std::string>& caFile)
        : certFile_(certFile), keyFile_(keyFile), keyPassFile_(keyPassFile), caFile_(caFile) {}
    const std::string& certFile() const { return certFile_; }
    const std::string& keyFile() const { return keyFile_; }
    const std::string& keyPassFile() const { return keyPassFile_; }
    const std::optional<std::string>& caFile() const { return caFile_; }
};

class FacilitatorConfig {
    std::string type_;
    std::string baseUrl_;
    std::optional<std::string> apiKeyFile_;
public:
    FacilitatorConfig(const std::string& type,
                     const std::string& baseUrl,
                     const std::optional<std::string>& apiKeyFile = std::nullopt)
        : type_(type), baseUrl_(baseUrl), apiKeyFile_(apiKeyFile) {}
    const std::string& type() const { return type_; }
    const std::string& baseUrl() const { return baseUrl_; }
    const std::optional<std::string>& apiKeyFile() const { return apiKeyFile_; }

    static ptr<FacilitatorConfig> createFomJson(const nlohmann::json& j);

};

class ServerConfig {
    bool httpEnabled_;
    bool httpsEnabled_;
    std::optional<uint16_t> httpPort_;
    std::optional<uint16_t> httpsPort_;
    std::string bindIp_;
    std::optional<TlsConfig> tls_;
public:
    ServerConfig(bool httpEnabled,
                 bool httpsEnabled,
                 std::optional<uint16_t> httpPort,
                 std::optional<uint16_t> httpsPort,
                 const std::string& bindIp,
                 const TlsConfig& tls)
        : httpEnabled_(httpEnabled), httpsEnabled_(httpsEnabled),
          httpPort_(httpPort), httpsPort_(httpsPort),
          bindIp_(bindIp), tls_(tls) {
        if (!httpEnabled_ && !httpsEnabled_) {
            throw std::invalid_argument("At least one protocol (HTTP or HTTPS) must be enabled in the server configuration.");
        }
        if (httpEnabled_ && !httpPort_) {
            throw std::invalid_argument("HTTP is enabled but httpPort is not set.");
        }
        if (httpPort_ && *httpPort_ == 0) {
            throw std::invalid_argument("HTTP port is set to zero, which is invalid.");
        }
        if (httpsEnabled_ && !httpsPort_) {
            throw std::invalid_argument("HTTPS is enabled but httpsPort is not set.");
        }
        if (httpsPort_ && *httpsPort_ == 0) {
            throw std::invalid_argument("HTTPS port is set to zero, which is invalid.");
        }
        if (bindIp_.empty()) {
            throw std::invalid_argument("bindIp cannot be empty.");
        }
        static const std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        static const std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
        if (!std::regex_match(bindIp_, ipv4_regex) && !std::regex_match(bindIp_, ipv6_regex) && bindIp_ != "0.0.0.0" && bindIp_ != "::") {
            throw std::invalid_argument("bindIp is not a valid IPv4 or IPv6 address.");
        }
    }
    bool httpEnabled() const { return httpEnabled_; }
    bool httpsEnabled() const { return httpsEnabled_; }
    const std::optional<uint16_t>& httpPort() const { return httpPort_; }
    const std::optional<uint16_t>& httpsPort() const { return httpsPort_; }
    const std::string& bindIp() const { return bindIp_; }
    const std::optional<TlsConfig>& tls() const { return tls_; }
};


enum class LogLevel {
    trace, debug, info, warn, error, fatal
};

enum class LogType {
    plain, json
};

inline LogLevel parseLogLevel(const std::string& level) {
    if (level == "trace") return LogLevel::trace;
    if (level == "debug") return LogLevel::debug;
    if (level == "info") return LogLevel::info;
    if (level == "warn") return LogLevel::warn;
    if (level == "error") return LogLevel::error;
    if (level == "fatal") return LogLevel::fatal;
    throw std::invalid_argument("Invalid log level: " + level);
}

inline LogType parseLogType(const std::string& type) {
    if (type == "plain") return LogType::plain;
    if (type == "json") return LogType::json;
    throw std::invalid_argument("Invalid log type: " + type);
}

class LogConfig {
    LogLevel level_;
    LogType type_;
public:
    LogConfig(const std::string& level, const std::string& type)
        : level_(parseLogLevel(level)), type_(parseLogType(type)) {}
    LogConfig(LogLevel level, LogType type)
        : level_(level), type_(type) {}
    LogLevel level() const { return level_; }
    LogType type() const { return type_; }

    static ptr<LogConfig> createDefault() {
        return std::make_shared<LogConfig>("info", "plain");
    }

    static ptr<LogConfig> createFromJson(const nlohmann::json& j);
};

class MachinePayConfig {
    ptr<ServerConfig> server_;
    ptr<FacilitatorConfig> facilitator_;
    ptr<LogConfig> log_;
public:
    MachinePayConfig(const ptr<ServerConfig>& server,
                     const ptr<FacilitatorConfig>& facilitator,
                     const ptr<LogConfig>& log)
        : server_(server), facilitator_(facilitator), log_(log) {
        CHECK_STATE(server_);
        CHECK_STATE(facilitator_);
        CHECK_STATE(log_);
    }
    const ptr<ServerConfig>& server() const {
        CHECK_STATE(server_);
        return server_;
    }
    const ptr<FacilitatorConfig>& facilitator() const {
        CHECK_STATE(facilitator_);
        return facilitator_;
    }

    const ptr<LogConfig>& log() const {
        CHECK_STATE(log_);
        return log_;
    }

    static ptr<MachinePayConfig> createFromJson(const nlohmann::json& j);
};
