#pragma once


#include "config/MachinePayConfig.h"
#include <string>
#include <cstdint>
#include <memory>



namespace proxygen {
    class HTTPServer;
}

class ServerFactory {
public:
    static std::shared_ptr<proxygen::HTTPServer> createServerInstance(
        ServerConfig& serverConfig );
};
