#include "HTTPConfig.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "config/JsonUtils.h"
#include "filesystem/FileManager.h"

HTTPConfig::HTTPConfig( bool isEnabled, uint16_t port ) : isEnabled_( isEnabled ), port_( port ) {
    CHECK_STATE( port_ > 0 );
}

bool HTTPConfig::isEnabled() const {
    return isEnabled_;
}
uint16_t HTTPConfig::port() const {
    return port_;
}

ptr< HTTPConfig > HTTPConfig::createFromJson(
    const nlohmann::json& j, ptr< FileManager > fileManager ) {
    try {
        CHECK_STATE( fileManager );
        CHECK_STATE( j.is_object() );
        bool enabled = JsonUtils::getBoolWithDefault( j, "enabled", true );
        uint16_t port = JsonUtils::getUint16WithDefault( j, "port", 8080 );
        return ptr< HTTPConfig >( new HTTPConfig( enabled, port ) );
    } catch ( const std::exception& ex ) {
        RETHROW_NESTED;
    }
}