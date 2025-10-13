#include "MachinePayApp.h"
#include "x402_protocol/X402Processor.h"

MachinePayApp::MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv) {
    spdlog::info("Looking for config");
    configManager_  = ConfigManager::initManager(configValuesFromCliAndEnv);
    Init::initLogLevelFromConfig(configManager());
}

uint32_t MachinePayApp::runUntilExit()
{
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
            } catch (...) {
                onError(std::current_exception());
            }
        });
        serverThread.join();
        spdlog::info("Machinepay server exited normally");
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
    proxygenServer_->stop();
}
