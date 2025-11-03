#pragma once
#include "MachinePayCommon.h"
#include "crypto/EIP712Domain.h"
#include "crypto/EthAddress.h"
#include "facilitator_clients/EasyNetFacilitatorClient.h"

class FacilitatorClient;
class FacilitatorConfig;
class FileManager;

class NetworkConfig {
private:
    std::string name_;
    EthAddress walletAddress_;
    std::shared_ptr< FacilitatorConfig > facilitator_;
    ptr< EIP712Domain > eip712Domain_;
    ptr< EasyNetFacilitatorClient > facilitatorClient_;

    NetworkConfig( const std::string& name, const EthAddress& walletAddress,
        std::shared_ptr< FacilitatorConfig >& facilitator, ptr< EIP712Domain >& domain ); // moved implementation to cpp


public:


    ptr<EasyNetFacilitatorClient> facilitatorClient() const;


    // Accessor for facilitator client
    [[nodiscard]] std::string name() const;

    const std::shared_ptr< FacilitatorConfig >& facilitator() const {
        CHECK_STATE( facilitator_ );
        return facilitator_;
    }

    [[nodiscard]] EthAddress walletAddress() const { return walletAddress_; }

    [[nodiscard]] const ptr< EIP712Domain >& eip712Domain() const {
        CHECK_STATE( eip712Domain_ );
        return eip712Domain_;
    }

    static std::shared_ptr< NetworkConfig > createFromJson(
        const nlohmann::json& j, ptr< FileManager > fileManager );

    string getTokenVersion( const string& tokenName ) const;

    string getTokenAddress( const string& tokenName ) const ;
};