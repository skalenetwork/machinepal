#include "NetworkConfig.h"
#include "FacilitatorConfig.h"
#include "common.h"

std::shared_ptr<NetworkConfig> NetworkConfig::fromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    std::string name = j.value("name", "machinepay-easy-test");
    std::set<std::string> supportedNetworks = {
        "machinepay-easy-testnet",
        "base-sepolia",
        "base"
    };
    CHECK_STATE2(supportedNetworks.contains(name), "Unsupported network name in config:" + name);
    std::shared_ptr<FacilitatorConfig> facilitator = nullptr;
    if (j.contains("facilitator") && j["facilitator"].is_object()) {
        facilitator = FacilitatorConfig::createFomJson(j["facilitator"], fileManager);
    }
    return std::make_shared<NetworkConfig>(name, facilitator);
}

