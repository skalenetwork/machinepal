#include "FacilitatorConfig.h"

FacilitatorConfig::FacilitatorConfig(const std::string& type,
                                     const std::string& baseUrl,
                                     const std::optional<std::filesystem::path>& apiKeyFile)
    : type_(type), baseUrl_(baseUrl), apiKeyFile_(apiKeyFile) {}

const std::string& FacilitatorConfig::type() const { return type_; }
const std::string& FacilitatorConfig::baseUrl() const { return baseUrl_; }
const std::optional<std::filesystem::path>& FacilitatorConfig::apiKeyFile() const { return apiKeyFile_; }

