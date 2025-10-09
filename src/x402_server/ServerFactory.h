#pragma once


#include "config/MachinePayConfig.h"
#include <string>
#include <cstdint>
#include <memory>
#include <proxygen/httpserver/HTTPServer.h>
#include <wangle/ssl/SSLContextConfig.h>


namespace proxygen {
    class HTTPServer;
}

class ServerFactory {
public:
    static wangle::SSLContextConfig createAndValidateWangleSSLContext(ptr<HTTPSConfig> https, std::string caFilePath);

    static void addHttpServerToIPConfigs(const ServerConfig &serverConfig,
                                         std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

    static void addHTTPSServerToConfigs(const ServerConfig &serverConfig,
                                        std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

    static std::shared_ptr<proxygen::HTTPServer> createServerInstance(
        const ServerConfig& serverConfig );
};
