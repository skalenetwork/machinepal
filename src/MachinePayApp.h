#pragma once
#include <map>
#include <string>
#include <spdlog/spdlog.h>
#include "config/MachinePayConfigManager.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"

class MachinePayApp {
public:
    MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv);
};

