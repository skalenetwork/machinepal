#include "MachinePayApp.h"
#include "x402_protocol/X402Processor.h"
#include <csignal>
#include <atomic>

MachinePayApp::MachinePayApp(const std::map<std::string, std::string>& configValuesFromCliAndEnv) {
    spdlog::info("Looking for config");
    configManager_  = ConfigManager::initManager(configValuesFromCliAndEnv);
    Init::initLogLevelFromConfig(configManager());
}

void MachinePayApp::onSuccess() {
    std::lock_guard<std::mutex> lock(exitMutex);
    exitCode_ = 0;
    spdlog::info("Machine pay server started successfully.");
};

void MachinePayApp::onError(std::exception_ptr eptr) {
    std::lock_guard<std::mutex> lock(exitMutex);
    exitCode_ = 1;
    try {
        if (eptr) std::rethrow_exception(eptr);
    } catch (const std::exception& ex) {
        spdlog::error("Proxygen server failed: {}", ex.what());
        {

            exitErrorMessage_ = ex.what();
        }
    } catch (...) {
        spdlog::error("Proxygen server failed: unknown error");
    }
};

static std::atomic<int> sigTermReceived{0};



static void terminateSignalHandler(int sig) {
        sigTermReceived = sig;
}

uint32_t MachinePayApp::runUntilExit() {

    std::signal(SIGTERM, terminateSignalHandler);
    std::signal(SIGINT, terminateSignalHandler);


    try {
        spdlog::info("Creating and starting server");
        serverFactory_ = std::make_shared<ServerFactory>(*this);
        auto serverConfig = configManager_->latestConfig()->server();

        spdlog::info("Creating server insatance");
        proxygenServer_ = serverFactory_->createServerInstance(*serverConfig);

        spdlog::info("Creating thread pool");
        auto ioExecutor = std::make_shared<folly::IOThreadPoolExecutor>(
        256,
        std::make_shared<folly::NamedThreadFactory>("x402Processor"));

        spdlog::info("Starting server");

        auto onSuccess = []() {
            spdlog::info("Machine pay server started successfully.");
        };
        auto onError = [](std::exception_ptr eptr) {
            try {
                if (eptr) std::rethrow_exception(eptr);
            } catch (const std::exception& ex) {
                spdlog::error("Proxygen server failed to start: {}", ex.what());
            } catch (...) {
                spdlog::error("Proxygen server failed to start: unknown error");
            }
        };

        std::thread serverThread([this, ioExecutor, onSuccess, onError]() {
            try {
                proxygenServer_->start(onSuccess, onError, nullptr, ioExecutor);
                setExited();
            } catch (...) {
                spdlog::error("Proxygen server failed to start: unknown error");
                setExited(1, "Unknown error starting server");
            }
        });

        while (!isExited() && !sigTermReceived) {
            usleep(100 * 1000);
        }
        if (sigTermReceived) {
            if (sigTermReceived == SIGINT)
                spdlog::info("SIGINT (Ctrl-C) received, stopping server.");
            else if (sigTermReceived == SIGTERM)
                spdlog::info("SIGTERM received, stopping server.");
            else {
                CHECK_STATE2(false, std::string("Unexpected signal {}") + to_string(sigTermReceived.load()));
            }
            stopServer();        }
        // Wait for server to exit after stopServer is called
        while (!isExited()) {
            usleep(100 * 1000);
        }


        serverThread.join();
        if (exitCode_ != 0) {
            spdlog::error("Error running machinepay server: {}. Server exited.", exitErrorMessage_);
            return exitCode_;
        }
        spdlog::info("Machinepay server exited normally.");
        return 0;
    } catch (const std::exception& ex)
    {
        spdlog::critical("Fatal error running machinepay server: {}. Server exited", ex.what());
        printNestedException(ex);
        return 1;
    }
}

void MachinePayApp::stopServer()
{
    static std::atomic<bool> alreadyStopped{false};
    if (alreadyStopped.exchange(true)) {
        return;
    }
    proxygenServer_->stop();
}

std::weak_ptr<MachinePayApp> MachinePayApp::sLatestInstance;
