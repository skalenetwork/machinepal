#include "ResourceConfig.h"

class FileManager;

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        CHECK_STATE(j.contains("location"));
        CHECK_STATE(j.at("location").is_string());
        std::string name = j.value("name", "");
        std::string location = j.value("location", "");

        CHECK_STATE(j.contains("type"));
        CHECK_STATE(j.at("type").is_string());
        ResourceType type = resourceTypeFromString(j.at("type").get<std::string>()) ;
        CHECK_STATE(j.contains("price"));
        CHECK_STATE(j.contains("token"));
        CHECK_STATE(j.at("price").is_string());
        CHECK_STATE(j.at("token").is_string());
        string price = j.at("price").get<std::string>();
        std::string token = j.at("token").get<std::string>();
        return ptr<ResourceConfig>(new ResourceConfig(name, location, type, price, token));
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}

ptr<vector<ptr<ResourceConfig> > > ResourceConfig::createVectorFromJsonArray(
    const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        auto result = std::make_shared<std::vector<ptr<ResourceConfig> > >();
        if (!j.contains("resources"))
            return result;
        auto resources = j.at("resources");
        CHECK_STATE(resources.is_array());
        for (const auto &item: resources) {
            auto res = createFromJson(item, fileManager);
            CHECK_STATE(res);
            result->push_back(res);
        }
        return result;
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}
