#pragma once


#include "MachinePayConfig.h"

class MachinePayConfigManager {


public:

    static void loadConfig(const std::string& yaml_path);

    static std::shared_ptr<MachinePayConfig> latestConfig();

private:

    static std::shared_ptr<MachinePayConfig> latestConfig_;

};