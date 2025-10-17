#pragma once
#include "common.h"

class FacilitatorConfig;
class FileManager;

class NetworkConfig {
    std::string name_;
    std::shared_ptr<FacilitatorConfig> facilitator_;
public:
    NetworkConfig(const std::string& name, std::shared_ptr<FacilitatorConfig> facilitator)
        : name_(name), facilitator_(std::move(facilitator)) {}
    const std::string& name() const { return name_; }
    const std::shared_ptr<FacilitatorConfig>& facilitator() const { return facilitator_; }

    static std::shared_ptr<NetworkConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};
