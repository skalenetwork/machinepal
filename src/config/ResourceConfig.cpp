#include "ResourceConfig.h"

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json& j) {
    std::string name = j.value("name", "");
    std::string location = j.value("location", "");
    ResourceType type = resourceTypeFromString(j.value("type", ""));
    return std::make_shared<ResourceConfig>(name, location, type);
}

ptr<vector<ptr<ResourceConfig>>> ResourceConfig::createVectorFromJsonArray(const nlohmann::json& j) {
    auto result = std::make_shared<std::vector<ptr<ResourceConfig>>>();
    if (!j.contains("resources"))
        return result;
    auto resources = j.at("resources");
    CHECK_STATE(resources.is_array());
    for (const auto& item : resources) {
        auto res = createFromJson(item);
        CHECK_STATE(res);
        result->push_back(res);
    }
    return result;
}
