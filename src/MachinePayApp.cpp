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
    x402Processor_ = std::make_shared<X402Processor>(*this);
    serverFactory_ = std::make_shared<ServerFactory>(*this);
    auto serverConfig = configManager_->latestConfig()->server();
    proxygenServer_ = serverFactory_->createServerInstance(*serverConfig);
    proxygenServer_->start();
    spdlog::info("Server exited");
}

void MachinePayApp::stopServer()
{
    proxygenServer_->stop();
}
