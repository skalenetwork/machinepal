#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "common.h"

enum class ResourceType {
    LocalFile,
    ApiJsonRpc,
    Unknown
};

inline ResourceType resourceTypeFromString(const std::string& typeStr) {
    if (typeStr == "local_file") return ResourceType::LocalFile;
    if (typeStr == "api-jsonrpc") return ResourceType::ApiJsonRpc;
    return ResourceType::Unknown;
}

class ResourceConfig {
    std::string name_;
    std::string location_;
    ResourceType type_;
public:
    ResourceConfig(const std::string& name, const std::string& location, ResourceType type)
        : name_(name), location_(location), type_(type) {}
    const std::string& name() const { return name_; }
    const std::string& location() const { return location_; }
    ResourceType type() const { return type_; }
    static ptr<ResourceConfig> createFromJson(const nlohmann::json& j);
    static ptr<vector<ptr<ResourceConfig>>> createResourcesFromJsonArray(const nlohmann::json& j) {
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

};
