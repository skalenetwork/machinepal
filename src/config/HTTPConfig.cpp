#include "HTTPConfig.h"
#include <stdexcept>

HTTPConfig::HTTPConfig(bool isEnabled, uint16_t port)
    : isEnabled_(isEnabled), port_(port)
{
    CHECK_STATE(port_ > 0);
}

bool HTTPConfig::isEnabled() const { return isEnabled_; }
uint16_t HTTPConfig::port() const { return port_; }

