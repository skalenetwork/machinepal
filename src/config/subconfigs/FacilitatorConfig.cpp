#include "FacilitatorConfig.h"

#include "config/JsonUtils.h"
#include "examples/PaymentExamples.h"
#include "exceptions/JsonValidationException.h"
#include "filesystem/FileManager.h"
#include "url/URLUtils.h"


FacilitatorConfig::FacilitatorConfig(FacilitatorType type,
                                     std::string baseUrl,
                                     std::optional<CanonicalPath> apiKeyFile)
    : type_(type), baseUrl_(std::move(baseUrl)), apiKeyFile_(std::move(apiKeyFile)) {}

FacilitatorType FacilitatorConfig::type() const { return type_; }
const std::string& FacilitatorConfig::baseUrl() const { return baseUrl_; }
const std::optional<CanonicalPath>& FacilitatorConfig::apiKeyFile() const { return apiKeyFile_; }

optional<HttpError> FacilitatorConfig::settlePayment(const shared_ptr<PaymentPayload>& /*paymentPayload*/,
    std::string &settlementInfo) {
    settlementInfo = URLUtils::base64Encode(std::string(PaymentExamples::EXACT_UCDC_SETTLEMENT_RESPONSE_CB_SEPOLIA));
    return std::nullopt;
}


ptr<FacilitatorConfig> FacilitatorConfig::createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    try {
        CHECK_STATE(fileManager);
        CHECK_STATE(j.is_object());
        auto type = mustContainType(j);
        if (type == FacilitatorType::cdp)
        {
            // For cdp type, api_key_file is required
            auto base_url = JsonUtils::mustContainString(j, "base_url");
            auto userProvidedApiKeyFile = JsonUtils::mustContainString(j, "api_key_file");


            auto resolved = fileManager->checkFileExistsAndReadableAndResolve(userProvidedApiKeyFile);
            CanonicalPath apiKeyFile(resolved);

            return ptr<FacilitatorConfig>(new FacilitatorConfig(
                type,
                base_url,
                apiKeyFile
            ));

        } else {
            throw JsonValidationException("Unsupported facilitator type", j);
        }


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
