#pragma once
#include "MachinePayCommon.h"
#include "crypto/EIP712Domain.h"
#include "crypto/EthAddress.h"

class FacilitatorConfig;
class FileManager;

class NetworkConfig {
    std::string name_;
    EthAddress walletAddress_;
    std::shared_ptr<FacilitatorConfig> facilitator_;
    ptr<EIP712Domain> eip712Domain_;
    NetworkConfig(const std::string& name,
                  const EthAddress& walletAddress,
                  std::shared_ptr<FacilitatorConfig>& facilitator,
                  ptr<EIP712Domain>& domain)
        : name_(name), walletAddress_(walletAddress), facilitator_(facilitator), eip712Domain_(domain) {
            CHECK_STATE(eip712Domain_);
            CHECK_STATE(facilitator_);
        }
public:
    const std::string& name() const { return name_; }
    const std::shared_ptr<FacilitatorConfig>& facilitator() const {
        CHECK_STATE(facilitator_);
        return facilitator_;
    }

    [[nodiscard]] EthAddress walletAddress() const
    {
        return walletAddress_;
    }
    [[nodiscard]] const ptr<EIP712Domain>& eip712Domain() const {
        CHECK_STATE(eip712Domain_);
        return eip712Domain_;
    }

    static std::shared_ptr<NetworkConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);

    string getTokenVersion(const string &tokenName);

    string getTokenAddress(const string &tokenName);
};
