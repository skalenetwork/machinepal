#include "HTTPConfig.h"
#include <stdexcept>
#include <nlohmann/json.hpp>

HTTPConfig::HTTPConfig(bool isEnabled, uint16_t port)
    : isEnabled_(isEnabled), port_(port)
{
    CHECK_STATE(port_ > 0);
}

bool HTTPConfig::isEnabled() const { return isEnabled_; }
uint16_t HTTPConfig::port() const { return port_; }

ptr<HTTPConfig> HTTPConfig::createFromJson(const nlohmann::json& j) {
    try {
        CHECK_STATE(j.is_object());
        bool enabled = j.value("enabled", true);
        uint16_t port = j.value("port", 8080);
        return std::make_shared<HTTPConfig>(enabled, port);
    } catch (const std::exception& ex) {
        RETHROW_NESTED;
    }
}
