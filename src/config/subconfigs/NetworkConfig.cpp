#include "NetworkConfig.h"
#include "FacilitatorConfig.h"
#include "MachinePayCommon.h"

// Constructor moved from header
NetworkConfig::NetworkConfig( const std::string& name, const EthAddress& walletAddress,
    ptr< FacilitatorConfig >& facilitator, ptr< EIP712Domain >& domain )
    : name_( name ),
      walletAddress_( walletAddress ),
      facilitator_( facilitator ),
      eip712Domain_( domain ) {
    CHECK_STATE( eip712Domain_ );
}


std::string NetworkConfig::name() const {
    return name_;
}

std::shared_ptr< NetworkConfig > NetworkConfig::createFromJson(
    const nlohmann::json& j, ptr< FileManager > fileManager ) {
    try {
        CHECK_STATE( fileManager );

        CHECK_STATE_JSON(j.contains("network"), "Missing required 'network' config section", j);

        CHECK_STATE_JSON(
            j["network"].is_object(), "'network' config section must be an object", j );

        auto networkJson = j["network"];

        CHECK_STATE_JSON( networkJson.contains( "revenue_wallet_address" ),
            "Missing required 'revenue_wallet_address' in network config", networkJson );
        CHECK_STATE_JSON( networkJson["revenue_wallet_address"].is_string(),
            "'revenue_wallet_address' in network config must be a string", networkJson );

        auto walletAddressStr = networkJson["revenue_wallet_address"].get< std::string >();

        auto walletAddress = EthAddress::parseHexAddress( walletAddressStr );

        std::string name = networkJson.value( "name", "machinepay-easynet" );
        std::map< std::string, ptr< EIP712Domain > > supportedNetworks{
            { "machinepay-easynet", EIP712Domain::machinePayEasyNet() },
            { "base-sepolia", EIP712Domain::baseSepolia() }, { "base", EIP712Domain::baseMainnet() }
        };
        CHECK_STATE_JSON(
            supportedNetworks.contains( name ), "Unsupported network name in config:" + name, networkJson );
        // Select domain
        auto domain = supportedNetworks.at( name );
        ptr<FacilitatorConfig > facilitator = nullptr;
        if ( networkJson.contains( "facilitator" ) && networkJson["facilitator"].is_object() ) {
            facilitator =
                FacilitatorConfig::createFomJson( networkJson["facilitator"], fileManager );
        }

        if (name != "machinepay-easynet") {
            CHECK_STATE_JSON( facilitator, "Facilitator config is required", networkJson );
        }
        return ptr< NetworkConfig >(
            new NetworkConfig( name, walletAddress, facilitator, domain ) );
    } catch ( const std::exception& ex ) {
        RETHROW_NESTED;
    }
}


string NetworkConfig::getTokenVersion( const string& tokenName ) const {
    if ( tokenName == "USDC" ) {
        return "2";
    }
    return "";
}

string NetworkConfig::getTokenAddress( const string& tokenName ) const {
    if ( tokenName == "USDC" ) {
        return "0x036CbD53842c5426634e7929541eC2318f3dCF7e";
    }
    return "";
}