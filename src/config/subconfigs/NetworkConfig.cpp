#include "common.h"
#include "NetworkConfig.h"
#include "FacilitatorConfig.h"


std::shared_ptr<NetworkConfig> NetworkConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        CHECK_STATE(fileManager);

        if (!j.contains("network")) {
            return nullptr;
        }

        CHECK_STATE(j["network"].is_object());

        auto networkJson = j["network"];

        std::string name = networkJson.value("name", "machinepay-easy-test");
        std::set<std::string> supportedNetworks = {
            "machinepay-easy-testnet",
            "base-sepolia",
            "base"
        };
        CHECK_STATE2(supportedNetworks.contains(name), "Unsupported network name in config:" + name);
        std::shared_ptr<FacilitatorConfig> facilitator = nullptr;
        if (networkJson.contains("facilitator") && networkJson["facilitator"].is_object()) {
            facilitator = FacilitatorConfig::createFomJson(networkJson["facilitator"], fileManager);
        }
        return ptr<NetworkConfig>(new NetworkConfig(name, facilitator));
    } catch (const std::exception& ex) {
        RETHROW_NESTED;
    }
}


string NetworkConfig::getTokenVersion(const string& tokenName)  {
    if (tokenName == "USDC") {
        return "2";
    }
    return "";
}

string NetworkConfig::getTokenAddress(const string& tokenName)  {
    if (tokenName == "USDC") {
        return "0x036CbD53842c5426634e7929541eC2318f3dCF7e";
    }
    return "";
}

