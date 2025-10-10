#pragma once
#include <string>
#include <optional>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include "common.h"
#include "filesystem/FileManager.h"

class FacilitatorConfig {
    std::string type_;
    std::string baseUrl_;
    std::optional<std::filesystem::path> apiKeyFile_;
public:
    FacilitatorConfig(const std::string& type,
                      const std::string& baseUrl,
                      const std::optional<std::filesystem::path>& apiKeyFile = std::nullopt);
    const std::string& type() const;
    const std::string& baseUrl() const;
    const std::optional<std::filesystem::path>& apiKeyFile() const;
    static ptr<FacilitatorConfig> createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};

