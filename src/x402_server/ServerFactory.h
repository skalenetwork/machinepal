#pragma once


#include "config/MachinePayConfig.h"
#include <proxygen/httpserver/HTTPServer.h>


namespace proxygen {
    class HTTPServer;
}

class MachinePayApp;

class ServerFactory {

    MachinePayApp& app_;

public:
    explicit ServerFactory(MachinePayApp &app)
        : app_(app) {
    }

    std::shared_ptr<proxygen::HTTPServer> createServerInstance(
            const ServerConfig& serverConfig );

private:
    static void addHttpServerToIPConfigs(const ServerConfig &serverConfig,
                                         std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

    static void addHTTPSServerToIpConfigs(const ServerConfig &serverConfig,
                                        std::vector<proxygen::HTTPServer::IPConfig>& ipConfigs);

};
