#pragma once


#include "MachinePayConfig.h"
#include <mutex>

class MachinePayConfigManager {
public:

    static void loadConfig(const std::string& yaml_path);

    static std::shared_ptr<MachinePayConfig> latestConfig();

private:

    static std::shared_ptr<MachinePayConfig> latestConfig_;
    static std::mutex latestConfigMutex_;

};