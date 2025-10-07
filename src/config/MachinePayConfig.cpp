//
// Created by kladko on 9/29/25.
//

#include "MachinePayConfig.h"
#include "MachinePayConfigLoader.h"


ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json& j) {
    CHECK_STATE(j.is_object());
    return std::make_shared<LogConfig>(
MachinePayConfigLoader::getStringWithDefault(j, "level", "info"),
MachinePayConfigLoader::getStringWithDefault(j, "type", "plain"));
}