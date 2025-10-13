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

class OrganizationConfig {
    ptr<ServerConfig> server_;
    std::string organizationName_;
public:
    OrganizationConfig(const ptr<ServerConfig>& server, const std::string& organizationName)
        : server_(server), organizationName_(organizationName) {}
    const ptr<ServerConfig>& server() const { return server_; }
    const std::string& organizationName() const { return organizationName_; }
    static ptr<OrganizationConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};
