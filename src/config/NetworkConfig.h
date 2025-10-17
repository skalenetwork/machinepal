#pragma once
#include <string>
#include <memory>
#include "FacilitatorConfig.h"
#include "json.hpp"

class NetworkConfig {
    std::string name_;
    std::shared_ptr<FacilitatorConfig> facilitator_;
public:
    NetworkConfig(const std::string& name, std::shared_ptr<FacilitatorConfig> facilitator)
        : name_(name), facilitator_(std::move(facilitator)) {}
    const std::string& name() const { return name_; }
    const std::shared_ptr<FacilitatorConfig>& facilitator() const { return facilitator_; }

    static std::shared_ptr<NetworkConfig> fromJson(const nlohmann::json& j) {
        std::string name = j.value("name", "machinepay-easy-test");

        spd::set<string> supportedNetworks = {
            "machinepay-easy-testnet",
            "base-sepolia",
            "base"
        };

        CHECK_STATE2(supporteNetworks.contains(name), "Unsupported network name in config:" + name);

        std::shared_ptr<FacilitatorConfig> facilitator = nullptr;
        if (j.contains("facilitator") && j["facilitator"].is_object()) {
            facilitator = FacilitatorConfig::createFomJson(j["facilitator"], nullptr);
        }
        return std::make_shared<NetworkConfig>(name, facilitator);
    }
};
