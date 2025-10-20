#include "FacilitatorConfig.h"

#include "config/JsonUtils.h"
#include "exceptions/JsonValidationException.h"
#include "filesystem/FileManager.h"



FacilitatorConfig::FacilitatorConfig(const FacilitatorType type,
                                     const std::string& baseUrl,
                                     const std::optional<CanonicalPath>& apiKeyFile)
    : type_(type), baseUrl_(baseUrl), apiKeyFile_(apiKeyFile) {}

const FacilitatorType FacilitatorConfig::type() const { return type_; }
const std::string& FacilitatorConfig::baseUrl() const { return baseUrl_; }
const std::optional<CanonicalPath>& FacilitatorConfig::apiKeyFile() const { return apiKeyFile_; }


ptr<FacilitatorConfig> FacilitatorConfig::createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    try {
        CHECK_STATE(fileManager);
        CHECK_STATE(j.is_object());
        std::optional<CanonicalPath> apiKeyFile = std::nullopt;
        auto userProvidedApiKeyFile = JsonUtils::getStringIfExists(j, "api_key_file");
        if (userProvidedApiKeyFile.has_value())
        {
            auto resolved = fileManager->checkFileExistsAndReadableAndResolve(userProvidedApiKeyFile.value());
            apiKeyFile = CanonicalPath(resolved);
        }
        return ptr<FacilitatorConfig>(new FacilitatorConfig(
            mustContainType(j),
            JsonUtils::mustContainString(j, "base_url"),
            apiKeyFile
        ));
    }
    catch (const std::exception& ex)
    {
        RETHROW_NESTED;
    }
}


FacilitatorType FacilitatorConfig::mustContainType(const nlohmann::json& j) {
    auto typeString = JsonUtils::mustContainString(j, "type");
    if (typeString == "cdp") {
        return FacilitatorType::cdp;
    }
    if (typeString == "base") {
        return FacilitatorType::base;
    }
    throw JsonValidationException("Invalid facilitator type: " + typeString, j);
}

