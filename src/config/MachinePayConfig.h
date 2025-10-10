#pragma once
#include <memory>
#include "ServerConfig.h"
#include "FacilitatorConfig.h"
#include "LogConfig.h"
#include "common.h"

class MachinePayConfig {
    ptr<ServerConfig> server_;
    ptr<FacilitatorConfig> facilitator_;
    ptr<LogConfig> log_;
public:
    MachinePayConfig(const ptr<ServerConfig>& server,
                     const ptr<FacilitatorConfig>& facilitator,
                     const ptr<LogConfig>& log);
    const ptr<ServerConfig>& server() const;
    const ptr<FacilitatorConfig>& facilitator() const;
    const ptr<LogConfig>& log() const;
    static ptr<MachinePayConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};
