#pragma once
#include <map>
#include <string>
#include <spdlog/spdlog.h>
#include "config/ConfigManager.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"
#include "x402_protocol/X402Processor.h"



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

    std::shared_ptr<X402Processor> makeX402Processor(ptr<IResponseSender>& _responseSender)
    {
        return std::make_shared<X402Processor>(*this, _responseSender);;
    }

    explicit MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv);

    void onSuccess();

    void onError(std::exception_ptr eptr);

    uint32_t runUntilExit();
    void stopServer();
    MachinePayApp() = delete;
    MachinePayApp(const MachinePayApp&) = delete;
    MachinePayApp(MachinePayApp&&) = delete;
    MachinePayApp& operator=(const MachinePayApp&) = delete;
    MachinePayApp& operator=(MachinePayApp&&) = default;

private:
    ptr<ConfigManager> configManager_;
    ptr<ServerFactory> serverFactory_;
    ptr<proxygen::HTTPServer> proxygenServer_;
    bool isExited_ {false};
    uint32_t exitCode_{1};
    string exitErrorMessage_;

public:
    [[nodiscard]] bool isExited() const {
        return isExited_;
    }

    [[nodiscard]] uint32_t exitCode() const {
        return exitCode_;
    }

    [[nodiscard]] string exitErrorMessage() const {
        return exitErrorMessage_;
    }
};
