#pragma once
#include "common.h"
class FileManager;
class ServerConfig;
class LogConfig;
class NetworkConfig;

class OrganizationConfig;

class MachinePayConfig {
    ptr<ServerConfig> server_;
    ptr<LogConfig> log_;
    ptr<std::map<string,ptr<OrganizationConfig>>> organizationsByName_;
    ptr<std::map<string,ptr<OrganizationConfig>>> organizationsBySubdomain_;
    std::shared_ptr<NetworkConfig> network_;
    MachinePayConfig(const ptr<ServerConfig>& server,
                     const ptr<LogConfig>& log,
                     const ptr<std::vector<ptr<OrganizationConfig>> >& organizations,
                     std::shared_ptr<NetworkConfig> network);

public:

    const ptr<ServerConfig>& server() const;

    [[nodiscard]] ptr<std::map<string, ptr<OrganizationConfig>>> organizationsByName() const {
        return organizationsByName_;
    }

    const ptr<LogConfig>& log() const;
    const std::shared_ptr<NetworkConfig>& network() const;

    [[nodiscard]] ptr<std::map<string, ptr<OrganizationConfig>>> organizationsBySubdomain() const {
        return organizationsBySubdomain_;
    }

    static ptr<MachinePayConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);

    ptr<OrganizationConfig> getDefaultOrganization() const {
        CHECK_STATE(organizationsByName_);
        auto it = organizationsByName_->find("");
        CHECK_STATE(it != organizationsByName_->end());
        CHECK_STATE(it->second);
        return it->second;
    }
};

