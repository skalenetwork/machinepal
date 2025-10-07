#pragma once


#include "MachinePayConfig.h"
#include <shared_mutex>
#include <atomic>
#include <chrono>

class MachinePayConfigManager {
public:


    static void initManager(map<string, string> configValuesFromCliAndEnv);

    static void reloadConfig();

    static std::shared_ptr<MachinePayConfig> latestConfig();

    static std::chrono::system_clock::time_point latestConfigModificationTime();

    static const std::string& latestConfigSha256();



private:


    static void setConfigValuesFromCliAndEnv(const std::map<std::string, std::string>& values) {
        std::unique_lock lock(latestConfigMutex_);
        if (configValuesFromCliAndEnv_.contains("CONFIG")) {
            configPath_ = configValuesFromCliAndEnv_.at("CONFIG");
        } else {
            configPath_ = "machinepay.yml";
        }
    }


    static std::string configPath_;
    static std::shared_ptr<MachinePayConfig> latestConfig_;
    static std::shared_mutex latestConfigMutex_;
    static std::chrono::system_clock::time_point latestConfigModificationTime_;
    static std::string latestConfigHash_;
    static std::map<std::string, std::string> configValuesFromCliAndEnv_;


    static std::string computeBlakeHash(const std::string& filePath);

    static void checkExistsAndReadable(std::string configFile);


};