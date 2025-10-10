#pragma once
#include <map>
#include <string>
#include <spdlog/spdlog.h>
#include "config/ConfigManager.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"



class MachinePayApp {
public:
    [[nodiscard]] ptr<ConfigManager> configManager() const {
        CHECK_STATE(configManager_);
        return configManager_;
    }
    [[nodiscard]] ptr<ServerFactory> serverFactory() const {
        CHECK_STATE(serverFactory_);
        return serverFactory_;
    }

    explicit MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv);
    void runUntilExit();
    MachinePayApp() = delete;
    MachinePayApp(const MachinePayApp&) = delete;
    MachinePayApp(MachinePayApp&&) = delete;
    MachinePayApp& operator=(const MachinePayApp&) = delete;
    MachinePayApp& operator=(MachinePayApp&&) = default;

private:
    ptr<ConfigManager> configManager_;
    ptr<ServerFactory> serverFactory_;
    ptr<proxygen::HTTPServer> proxygenServer_;
};
