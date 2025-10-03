#pragma once


#include <string>
#include <cstdint>
#include <memory>

namespace proxygen {
    class HTTPServer;
}

class ServerFactory {
public:
    static std::shared_ptr<proxygen::HTTPServer> createServerInstance(
        std::string _bindIP, uint64_t _bindPort);
};
