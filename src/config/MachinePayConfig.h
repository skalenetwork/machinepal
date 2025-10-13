#pragma once
#include <memory>
#include "ServerConfig.h"
#include "FacilitatorConfig.h"
#include "LogConfig.h"
#include "common.h"

class OrganizationConfig;

class MachinePayConfig {
    ptr<ServerConfig> server_;
    ptr<FacilitatorConfig> facilitator_;
    ptr<LogConfig> log_;
    ptr<std::map<string,ptr<OrganizationConfig>>>  organizations_;
public:
    MachinePayConfig(const ptr<ServerConfig>& server,
                     const ptr<FacilitatorConfig>& facilitator,
                     const ptr<LogConfig>& log,
                     const ptr<std::vector<ptr<OrganizationConfig>> >& organizations);
    const ptr<ServerConfig>& server() const;
    const ptr<FacilitatorConfig>& facilitator() const;
    const ptr<LogConfig>& log() const;
    static ptr<MachinePayConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);

    ptr<OrganizationConfig> getDefaultOrganization() const {
        CHECK_STATE(organizations_);
        auto it = organizations_->find("");
        CHECK_STATE(it != organizations_->end());
        CHECK_STATE(it->second);
        return it->second;
    }
};
