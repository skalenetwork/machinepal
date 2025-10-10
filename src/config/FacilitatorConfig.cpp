#include "FacilitatorConfig.h"

FacilitatorConfig::FacilitatorConfig(const std::string& type,
                                     const std::string& baseUrl,
                                     const std::optional<CanonicalPath>& apiKeyFile)
    : type_(type), baseUrl_(baseUrl), apiKeyFile_(apiKeyFile) {}

const std::string& FacilitatorConfig::type() const { return type_; }
const std::string& FacilitatorConfig::baseUrl() const { return baseUrl_; }
const std::optional<CanonicalPath>& FacilitatorConfig::apiKeyFile() const { return apiKeyFile_; }
