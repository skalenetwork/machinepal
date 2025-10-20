#include "ResourceConfig.h"

#include "config/JsonUtils.h"
#include "exceptions/JsonValidationException.h"


class FileManager;


ResourceType ResourceConfig::mustContainType(const nlohmann::json &j) {
    auto typeString = JsonUtils::mustContainString(j, "type");
    if (typeString == "local_file") {
        return ResourceType::LocalFile;
    }
    if (typeString == "api-jsonrpc") {
        return ResourceType::ApiJsonRpc;
    }

    if (typeString == "api-rest") {
        return ResourceType::ApiRest;
    }

    throw JsonValidationException("Invalid resource type: " + typeString, j);
}

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        auto name = JsonUtils::mustContainString(j, "name");
        auto type = mustContainType(j);
        auto location = JsonUtils::mustContainString(j, "location");
        auto price = JsonUtils::mustContainPrice(j, "price");
        auto token = JsonUtils::mustContainString(j, "token");
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
        CHECK_STATE_JSON(resources.is_array(), "Resources must be an array resources", j);
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

