#pragma once


#include "MachinePayConfig.h"
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <memory>

class MachinePayConfigManager {
public:
    static MachinePayConfigManager& getInstance() {
        static MachinePayConfigManager instance;
        return instance;
    }

    void initManager(const std::map<std::string, std::string>& configValuesFromCliAndEnv);
    void reloadConfig();
    std::shared_ptr<MachinePayConfig> latestConfig();
    std::chrono::system_clock::time_point latestConfigModificationTime();
    const std::string& latestConfigSha256();



private:
    MachinePayConfigManager() = default;
    MachinePayConfigManager(const MachinePayConfigManager&) = delete;
    MachinePayConfigManager& operator=(const MachinePayConfigManager&) = delete;


    void setConfigValuesFromCliAndEnv(const std::map<std::string, std::string>& values);

    std::string userProvidedConfigPath_ = "machinepay.yml";
    std::string fullyResolvedConfigPath_;
    std::shared_ptr<MachinePayConfig> latestConfig_;
    std::shared_mutex latestConfigMutex_;
    std::chrono::system_clock::time_point latestConfigModificationTime_;
    std::string latestConfigHash_;
    std::map<std::string, std::string> configValuesFromCliAndEnv_;


    std::string computeBlakeHash(const std::string& filePath);
    void checkFileExistsAndReadable(const std::string& configFile);
};