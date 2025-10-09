#include "MachinePayApp.h"

MachinePayApp::MachinePayApp(std::map<std::string, std::string> configValuesFromCliAndEnv) {
    spdlog::info("Processing config");
    MachinePayConfigManager::initManager(configValuesFromCliAndEnv);
    Init::initLogLevelFromConfig();
    auto serverConfig = MachinePayConfigManager::getInstance()->latestConfig()->server();
    auto serverObject = ServerFactory::createServerInstance(*serverConfig);
    serverObject->start();
}

