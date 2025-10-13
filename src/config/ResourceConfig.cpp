#include "ResourceConfig.h"

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json& j) {
    std::string name = j.value("name", "");
    std::string location = j.value("location", "");
    std::string type = j.value("type", "");
    return std::make_shared<ResourceConfig>(name, location, type);
}

