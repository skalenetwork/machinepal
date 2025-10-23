#include "common.h"

#include "config/MachinePayConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ResourceConfig.h"
#include "PaymentRequirements.h"


#include "X402PaymentRequirements.h"

std::string X402PaymentRequirements::getPaymentRequirementsAsString(ptr<OrganizationConfig> organization,
                                                          ptr<ResourceConfig> resource,
                                                          ptr<MachinePayConfig> config) {

    CHECK_STATE(organization);
    CHECK_STATE(resource);
    CHECK_STATE(config);

    auto priceStr = resource->priceStr();
    auto scheme = resource->paymentScheme();
    auto mimeType = resource->mimeType();
    auto network = config->network()->name();
    auto payTo = organization->payToAddressAsString();
    auto maxTimeoutSeconds = 600;
    auto description = resource->description();
    auto tokenName = resource->token();
    auto asset = config->network()->getTokenAddress(tokenName);
    auto extraVersion = config->network()->getTokenVersion(tokenName);;
    //auto path = resource_->machinePayPath();
    nlohmann::json extra;
    extra["name"] = tokenName;
    if (!extraVersion.empty()) {
        extra["version"] = extraVersion;
    }
    PaymentRequirements req(
        scheme,
        network,
        priceStr,
        resource->location(),
        description,
        mimeType,
        std::nullopt, // outputSchema
        payTo,
        maxTimeoutSeconds,
        asset,
        extra
    );
    return *PaymentRequirements::toString(req);
}