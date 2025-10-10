#include "ServerConfig.h"
#include <stdexcept>
#include <regex>

static const std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
static const std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");

ServerConfig::ServerConfig(const std::string& bindIp,
                           ptr<HTTPConfig> http,
                           ptr<HTTPSConfig> https)
    : bindIp_(bindIp), http_(http), https_(https) {
    if (!http_ && !https_) {
        throw std::invalid_argument("At least one of HTTP or HTTPS configuration must be provided.");
    }
    if (!http_->isEnabled() && !https_->isEnabled()) {
        throw std::invalid_argument("At least one protocol (HTTP or HTTPS) must be enabled in the server configuration.");
    }
    if (bindIp_.empty()) {
        throw std::invalid_argument("bindIp cannot be empty.");
    }
    if (!std::regex_match(bindIp_, ipv4_regex) && !std::regex_match(bindIp_, ipv6_regex) && bindIp_ != "0.0.0.0" && bindIp_ != "::") {
        throw std::invalid_argument("bindIp is not a valid IPv4 or IPv6 address.");
    }
}

const std::string& ServerConfig::bindIp() const { return bindIp_; }
const ptr<HTTPConfig> ServerConfig::http() const { return http_; }
const ptr<HTTPSConfig> ServerConfig::https() const { return https_; }
