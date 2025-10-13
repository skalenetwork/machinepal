#include "MachinePayApp.h"
#include "x402_protocol/X402Processor.h"

MachinePayApp::MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv) {
    spdlog::info("Looking for config");
    configManager_  = ConfigManager::initManager(configValuesFromCliAndEnv);
    Init::initLogLevelFromConfig(configManager());
}

void MachinePayApp::runUntilExit()
{


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
    proxygenServer_->start(nullptr, nullptr, nullptr,  ioExecutor);
    spdlog::info("Server exited");
}

void MachinePayApp::stopServer()
{
    proxygenServer_->stop();
}
