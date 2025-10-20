#include "ServerConfig.h"
#include <stdexcept>
#include <regex>

#include "config/ConfigLoader.h"
#include "config/JsonUtils.h"

static const std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
static const std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");

ServerConfig::ServerConfig(const std::string& hostName, const std::string& bindIp,
                           ptr<HTTPConfig> http,
                           ptr<HTTPSConfig> https)
    : hostName_(hostName), bindIp_(bindIp), http_(http), https_(https) {



    if (hostName_.empty()) {
        throw std::invalid_argument("hostName cannot be empty.");
    }

    // Hostname validation: must be a valid local or internet hostname
    static const std::regex hostname_regex(R"(^([a-zA-Z0-9][-a-zA-Z0-9]{0,62})(\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})*$)");
    if (!std::regex_match(hostName_, hostname_regex)) {
        throw std::invalid_argument("hostName is not a valid local or internet hostname." + hostName_);
    }


    if (bindIp_.empty()) {
        throw std::invalid_argument("bindIp cannot be empty.");
    }
    if (!std::regex_match(bindIp_, ipv4_regex) && !std::regex_match(bindIp_, ipv6_regex) && bindIp_ != "0.0.0.0" && bindIp_ != "::") {
        throw std::invalid_argument("bindIp is not a valid IPv4 or IPv6 address.");
    }

    if (!http_ && !https_) {
        throw std::invalid_argument("At least one of HTTP or HTTPS configuration must be provided.");
    }
    if (!http_->isEnabled() && !https_->isEnabled()) {
        throw std::invalid_argument("At least one protocol (HTTP or HTTPS) must be enabled in the server configuration.");
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

        return ptr<ServerConfig>(new ServerConfig(
            JsonUtils::getStringWithDefault(j, "hostname", ""),
            JsonUtils::getStringWithDefault(j, "bind_ip", "0.0.0.0"),
            httpConfig,
            httpsConfig
        ));
    } catch (exception& ex)
    {
        RETHROW_NESTED;
    }
}

const std::string& ServerConfig::bindIp() const { return bindIp_; }
const ptr<HTTPConfig> ServerConfig::http() const { return http_; }
const ptr<HTTPSConfig> ServerConfig::https() const { return https_; }
