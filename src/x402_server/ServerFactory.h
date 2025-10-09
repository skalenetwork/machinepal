#pragma once


#include "config/MachinePayConfig.h"
#include <string>
#include <cstdint>
#include <memory>
#include <proxygen/httpserver/HTTPServer.h>


namespace proxygen {
    class HTTPServer;
}

class ServerFactory {

public:

    static std::shared_ptr<proxygen::HTTPServer> createServerInstance(
            const ServerConfig& serverConfig );

private:
    static void addHttpServerToIPConfigs(const ServerConfig &serverConfig,
                                         std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

    static void addHTTPSServerToIpConfigs(const ServerConfig &serverConfig,
                                        std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

};
