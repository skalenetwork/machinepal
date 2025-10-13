#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "common.h"

class ResourceConfig {
    std::string name_;
    std::string location_;
    std::string type_;
public:
    ResourceConfig(const std::string& name, const std::string& location, const std::string& type)
        : name_(name), location_(location), type_(type) {}
    const std::string& name() const { return name_; }
    const std::string& location() const { return location_; }
    const std::string& type() const { return type_; }
    static ptr<ResourceConfig> createFromJson(const nlohmann::json& j);
};
