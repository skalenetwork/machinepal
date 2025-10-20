#pragma once
#include "common.h"
#include "filesystem/FileManager.h"

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
    std::string price_;
    std::string token_;

    ResourceConfig(const std::string& name, const std::string& location, ResourceType type, const std::string& price, const std::string& token)
        : name_(name), location_(location), type_(type), price_(price), token_(token) {}

public:
    const std::string& name() const { return name_; }
    const std::string& location() const { return location_; }
    ResourceType type() const { return type_; }
    const std::string& price() const { return price_; }
    const std::string& token() const { return token_; }
    static ptr<ResourceConfig> createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager);
    static ptr<vector<ptr<ResourceConfig>>> createVectorFromJsonArray(const nlohmann::json &j, ptr<FileManager> fileManager);

};
