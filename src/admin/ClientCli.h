

#pragma once

#include <string>
#include "CLI/CLI.hpp"


namespace proxygen {
    enum class HTTPMethod;
}

class ClientConfig {
public:
    std::string url;
    proxygen::HTTPMethod method;
    std::string payload;
};

namespace ClientCli {

    void addClientSubcommand(CLI::App& app, ClientConfig& config);
    int runClientCommand(const ClientConfig& config);

}
