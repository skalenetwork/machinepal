#include "NetworkConfig.h"
#include "FacilitatorConfig.h"
#include "MachinePayCommon.h"

// Constructor moved from header
NetworkConfig::NetworkConfig( const std::string& name, const EthAddress& walletAddress,
    optional<ptr< FacilitatorConfig >>& facilitator, ptr< EIP712Domain >& domain )
    : name_( name ),
      walletAddress_( walletAddress ),
      facilitator_( facilitator ),
      eip712Domain_( domain ) {
    if (facilitator_.has_value()) {
        CHECK_STATE( facilitator_.value() );
    }
    CHECK_STATE( eip712Domain_ );
}


std::string NetworkConfig::name() const {
    return name_;
}

std::shared_ptr< NetworkConfig > NetworkConfig::createFromJson(
    const nlohmann::json& j, ptr< FileManager > fileManager ) {
    try {
        CHECK_STATE( fileManager );

        if ( !j.contains( "network" ) ) {
            return nullptr;
        }

        CHECK_STATE( j["network"].is_object() );

        auto networkJson = j["network"];

        CHECK_STATE( networkJson.contains( "wallet_address" ) );
        CHECK_STATE( networkJson["wallet_address"].is_string() );

        auto walletAddressStr = networkJson["wallet_address"].get< std::string >();

        auto walletAddress = EthAddress::parseHexAddress( walletAddressStr );

        std::string name = networkJson.value( "name", "machinepay-easynet" );
        std::map< std::string, ptr< EIP712Domain > > supportedNetworks{
            { "machinepay-easynet", EIP712Domain::machinePayEasyNet() },
            { "base-sepolia", EIP712Domain::baseSepolia() }, { "base", EIP712Domain::baseMainnet() }
        };
        CHECK_STATE2(
            supportedNetworks.contains( name ), "Unsupported network name in config:" + name );
        // Select domain
        auto domain = supportedNetworks.at( name );
        optional<ptr<FacilitatorConfig >> facilitator = nullopt;
        if ( networkJson.contains( "facilitator" ) && networkJson["facilitator"].is_object() ) {
            facilitator =
                FacilitatorConfig::createFomJson( networkJson["facilitator"], fileManager );
        }

        if (name != "machinepay-easynet") {
            CHECK_STATE_JSON( facilitator, "Facilitator config is required",
                json);
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