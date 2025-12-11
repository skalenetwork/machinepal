

#pragma once

#include <string>
#include "CLI/CLI.hpp"

struct ClientConfig;

namespace ClientCli {

    void addClientSubcommand(CLI::App& app, ClientConfig& config);

}
