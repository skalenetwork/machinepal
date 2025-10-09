#pragma once
#include <map>
#include <string>
#include <spdlog/spdlog.h>
#include "config/MachinePayConfigManager.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"

class MachinePayApp {
public:
    [[nodiscard]] ptr<MachinePayConfigManager> configManager() const {
        CHECK_STATE(configManager_);
        return configManager_;
    }
    [[nodiscard]] ptr<ServerFactory> serverFactory() const {
        CHECK_STATE(serverFactory_);
        return serverFactory_;
    }

    explicit MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv);
    MachinePayApp(const MachinePayApp&) = delete;
    MachinePayApp(MachinePayApp&&) = delete;
    MachinePayApp& operator=(const MachinePayApp&) = delete;
    MachinePayApp& operator=(MachinePayApp&&) = default;

private:
    ptr<MachinePayConfigManager> configManager_;
    ptr<ServerFactory> serverFactory_;
};
