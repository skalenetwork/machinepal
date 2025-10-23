#include "MachinePayCommon.h"
#include "PaymentRequiredResponse.h"

#include "config/MachinePayConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ResourceConfig.h"

using json = nlohmann::json;

PaymentRequiredResponse PaymentRequiredResponse::fromJson(const json &j) {
    PaymentRequiredResponse out;
    if (j.contains("x402Version") && j["x402Version"].is_number_integer()) {
        out = PaymentRequiredResponse();
        out = PaymentRequiredResponse( {});
    }
    // accepts (array of PaymentRequirements)
    if (j.contains("accepts") && j["accepts"].is_array()) {
        std::vector<PaymentRequirements> vec;
        for (const auto &elem : j["accepts"]) {
            // PaymentRequirements::fromJson returns shared_ptr
            auto pr = PaymentRequirements::fromJson(elem);
            if (pr) vec.push_back(*pr);
        }
        if (out.x402Version() == 0) {
            // construct with default version 1 if not set earlier
            out = PaymentRequiredResponse( vec);
        } else {
            out = PaymentRequiredResponse( vec);
        }
    }
    // error
    if (j.contains("error") && !j["error"].is_null()) {
        auto err = j["error"].get<std::string>();
    }

    return out;
}

json PaymentRequiredResponse::toJson() const {
    json j;
    j["x402Version"] = x402Version_;
    j["accepts"] = json::array();
    for (const auto &p : accepts_) {
        j["accepts"].push_back(p.toJson());
    }

    j["error"] = error_;

    return j;

}

std::string PaymentRequiredResponse::getPaymentRequirementsAsString(ptr<OrganizationConfig> organization,
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

    std::vector<PaymentRequirements> reqs({req});

    auto response = PaymentRequiredResponse(reqs);
    return response.toJson().dump();
}