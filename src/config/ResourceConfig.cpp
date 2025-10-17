#include "ResourceConfig.h"

class FileManager;

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    CHECK_STATE(fileManager);
    std::string name = j.value("name", "");
    std::string location = j.value("location", "");
    ResourceType type = resourceTypeFromString(j.value("type", ""));
    return ptr<ResourceConfig>(new ResourceConfig(name, location, type));
}

ptr<vector<ptr<ResourceConfig>>> ResourceConfig::createVectorFromJsonArray(const nlohmann::json& j, ptr<FileManager> fileManager) {
    CHECK_STATE(fileManager);
    auto result = std::make_shared<std::vector<ptr<ResourceConfig>>>();
    if (!j.contains("resources"))
        return result;
    auto resources = j.at("resources");
    CHECK_STATE(resources.is_array());
    for (const auto& item : resources) {
        auto res = createFromJson(item, fileManager);
        CHECK_STATE(res);
        result->push_back(res);
    }
    return result;
}
