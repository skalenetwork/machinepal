#pragma once

#include "MachinePayCommon.h"
#include "MachinePayConfig.h"


class ConfigManager {
public:



    static ptr<ConfigManager> initManager(const std::map<std::string, std::string>& configValuesFromCliAndEnv);
    static ptr<ConfigManager> createInstance(const std::map<std::string, std::string>& configValuesFromCliAndEnv);

    void reloadConfig();
    std::shared_ptr<MachinePayConfig> latestConfig();
    std::chrono::system_clock::time_point latestConfigModificationTime();
    const std::string& latestConfigSha256();


    [[nodiscard]] ptr<FileManager> fileManager() const {
        CHECK_STATE(fileManager_);
        return fileManager_;
    }


public:
    ConfigManager() = default;

private:

    ptr<FileManager> fileManager_;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;


    void initConfigFilePathUsingConfigValuesFromCliAndEnv(const std::map<std::string, std::string>& values);

    std::shared_ptr<MachinePayConfig> latestConfig_;
    std::shared_mutex latestConfigMutex_;
    std::chrono::system_clock::time_point latestConfigModificationTime_;
    std::string latestConfigHash_;
    std::map<std::string, std::string> configValuesFromCliAndEnv_;


    std::string computeBlakeHash(const filesystem::path& filePath);
    void checkFileExistsAndReadable(const std::string& configFile);
};
