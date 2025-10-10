#include "MachinePayApp.h"

MachinePayApp::MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv) {
    spdlog::info("Looking for config");
    configManager_  = ConfigManager::initManager(configValuesFromCliAndEnv);
    Init::initLogLevelFromConfig(configManager());
    auto serverConfig = configManager_->latestConfig()->server();
    serverFactory_ = std::make_shared<ServerFactory>(*this);
    auto serverObject = serverFactory_->createServerInstance(*serverConfig);
    serverObject->start();
}

