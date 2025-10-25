#pragma once
#include "MachinePayCommon.h"
#include "crypto/Address.h"

class FacilitatorConfig;
class FileManager;

class NetworkConfig {
    std::string name_;
    Address walletAddress_;
    std::shared_ptr<FacilitatorConfig> facilitator_;
    NetworkConfig(const std::string& name, const Address& walletAddress, std::shared_ptr<FacilitatorConfig>& facilitator)
        : name_(name), walletAddress_(walletAddress), facilitator_(facilitator) {}
public:
    const std::string& name() const { return name_; }
    const std::shared_ptr<FacilitatorConfig>& facilitator() const { return facilitator_; }

    [[nodiscard]] Address walletAddress() const
    {
        return walletAddress_;
    }

    static std::shared_ptr<NetworkConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);

    string getTokenVersion(const string &tokenName);

    string getTokenAddress(const string &tokenName);
};
