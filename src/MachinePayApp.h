#pragma once
#include <spdlog/spdlog.h>
#include "config/ConfigManager.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"
#include "x402_protocol/X402Processor.h"
#include "payment/PaymentManager.h"


class MachinePayDB;

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

    [[nodiscard]] ptr<PaymentManager> paymentManager() const {
        CHECK_STATE(paymentManager_);
        return paymentManager_;
    }

    [[nodiscard]] ptr<MachinePayDB> machinePayDB() const {
        CHECK_STATE(machinePayDB_);
        return machinePayDB_;
    }


    std::shared_ptr<X402Processor> makeX402Processor(ptr<IResponseSender> &_responseSender) {
        return std::make_shared<X402Processor>(*this, _responseSender);;
    }


    static std::weak_ptr<MachinePayApp> sLatestInstance;

    static ptr<MachinePayApp> makeInstance(std::map<std::string, std::string> &configValuesFromCliAndEnv) {
        auto shared = ptr<MachinePayApp>(new MachinePayApp(configValuesFromCliAndEnv));
        sLatestInstance = shared;
        CHECK_STATE(shared);
        return shared;
    }

    static void processCRTLC() noexcept {
        try {
            auto shared = sLatestInstance.lock();
            if (shared) {
                shared->stopServer();
            } else {
                spdlog::warn("No MachinePayApp instance to stop server on terminate signal.");
            }
        } catch (const std::exception &ex) {
            spdlog::error("Error stopping server by terminate signal: {}", ex.what());
        } catch (...) {
            spdlog::error("Unknown error stopping server by terminate signal.");
        }
    }

    void onSuccess();

    void onError(std::exception_ptr eptr);

    uint32_t runUntilExit();

    void stopServer();

    MachinePayApp() = delete;

    MachinePayApp(const MachinePayApp &) = delete;

    MachinePayApp(MachinePayApp &&) = delete;

    MachinePayApp &operator=(const MachinePayApp &) = delete;

    MachinePayApp &operator=(MachinePayApp &&) = default;


    [[nodiscard]] bool isStarted() const {
        return isStarted_.load();
    }


    [[nodiscard]] bool isExited() {
        std::lock_guard<std::mutex> lock(exitMutex);
        return isExited_;
    }

    void setExited(uint32_t exitCode = 0, const string &exitErrorMessage = "") {
        std::lock_guard<std::mutex> lock(exitMutex);
        isExited_ = true;
        exitCode_ = exitCode;
        exitErrorMessage_ = exitErrorMessage;
    }

    [[nodiscard]] uint32_t exitCode() {
        std::lock_guard<std::mutex> lock(exitMutex);
        return exitCode_;
    }

    [[nodiscard]] string exitErrorMessage() {
        std::lock_guard<std::mutex> lock(exitMutex);
        return exitErrorMessage_;
    }


    [[nodiscard]] std::filesystem::path configPath() const {
        return configPath_;
    }

private:
    explicit MachinePayApp(const std::map<std::string, std::string> &configValuesFromCliAndEnv);

    ptr<ConfigManager> configManager_;
    ptr<ServerFactory> serverFactory_;
    ptr<proxygen::HTTPServer> proxygenServer_;
    ptr<PaymentManager> paymentManager_;
    std::atomic<bool> isStarted_{false};
    std::atomic<bool> serverStopCalled_{false};


    ptr<MachinePayDB> machinePayDB_;


    bool isExited_{false};
    uint32_t exitCode_{0};
    string exitErrorMessage_;


    std::mutex exitMutex;
    std::filesystem::path configPath_;
};
