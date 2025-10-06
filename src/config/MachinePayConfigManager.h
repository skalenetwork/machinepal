#pragma once


#include "MachinePayConfig.h"
#include <shared_mutex>
#include <atomic>
#include <chrono>

class MachinePayConfigManager {
public:

    static void loadConfig(const std::string& yaml_path);

    static void reloadConfig();

    static std::shared_ptr<MachinePayConfig> latestConfig();

    static std::chrono::system_clock::time_point latestConfigMTime();

    static const std::string& latestConfigSha256();


private:

    static std::string configPath_;
    static std::shared_ptr<MachinePayConfig> latestConfig_;
    static std::shared_mutex latestConfigMutex_;
    static std::chrono::system_clock::time_point latestConfigModificationTime_;
    static std::string latestConfigHash_;


    static std::string computeBlakeHash(const std::string& filePath);

    static void reloadConfigUnsafe();
};