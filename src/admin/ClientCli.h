

#pragma once

#include <string>
#include "CLI/CLI.hpp"


class ClientConfig {
public:
    std::string url;
    std::string method;
    std::string payload;
};

namespace ClientCli {

    void addClientSubcommand(CLI::App& app, ClientConfig& config);
    int runClientCommand(const ClientConfig& config);

}
