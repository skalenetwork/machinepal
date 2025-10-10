#pragma once
#include <string>
#include <optional>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include "common.h"
#include "filesystem/FileManager.h"
#include "filesystem/CanonicalPath.h"

class FacilitatorConfig {
    std::string type_;
    std::string baseUrl_;
    std::optional<CanonicalPath> apiKeyFile_;
public:
    FacilitatorConfig(const std::string& type,
                      const std::string& baseUrl,
                      const std::optional<CanonicalPath>& apiKeyFile = std::nullopt);
    const std::string& type() const;
    const std::string& baseUrl() const;
    const std::optional<CanonicalPath>& apiKeyFile() const;
    static ptr<FacilitatorConfig> createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};
