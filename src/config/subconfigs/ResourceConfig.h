#pragma once
#include <memory>
#include <boost/multiprecision/cpp_int.hpp>

#include "common.h"
#include "filesystem/FileManager.h"
#include "url/URLUtils.h"

enum class ResourceType {
    LocalFile,
    ApiJsonRpc,
    ApiRest
};



class ResourceConfig {
public:
    [[nodiscard]] std::string machinePayPath() const {
        return machinePayPath_;
    }

private:
    std::string name_;
    std::string location_;
    std::string machinePayPath_;
    ResourceType type_;
    boost::multiprecision::uint256_t price_;
    std::string token_;

    ResourceConfig(const std::string& name, const std::string& location, ResourceType type, boost::multiprecision::uint256_t price, const std::string& token)
        : name_(name), location_(location), type_(type), price_(price), token_(token) {
        if (type_ == ResourceType::LocalFile) {
            machinePayPath_ = location_;
        } else {
            machinePayPath_ = URLUtils::getLocationFromUrl(location_);
        }
    }

public:
    const std::string& name() const { return name_; }
    const std::string& location() const { return location_; }
    ResourceType type() const { return type_; }
    const boost::multiprecision::uint256_t price() const { return price_; }
    const std::string& token() const { return token_; }


    static ResourceType mustContainType(const nlohmann::json &j);

    static ptr<ResourceConfig> createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager);
    static ptr<vector<ptr<ResourceConfig>>> createVectorFromJsonArray(const nlohmann::json &j, ptr<FileManager> fileManager);


};
