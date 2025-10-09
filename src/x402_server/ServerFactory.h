#pragma once


#include "config/MachinePayConfig.h"
#include <string>
#include <cstdint>
#include <memory>
#include <wangle/ssl/SSLContextConfig.h>


namespace proxygen {
    class HTTPServer;
}

class ServerFactory {
public:
    static wangle::SSLContextConfig createAndValidateWangleSSLContext(ptr<HTTPSConfig> https, std::string &caFilePath);

    static std::shared_ptr<proxygen::HTTPServer> createServerInstance(
        const ServerConfig& serverConfig );
};
