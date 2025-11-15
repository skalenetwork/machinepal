#pragma once

#include "FacilitatorClient.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EthAddress.h"
#include "db/EasyNetDb.h"

class PaymentRequirements;
// A local in-process facilitator implementation that uses EasyNetDb instead of
// making remote HTTP calls. Intended for development, testing, or offline mode.
//
// verify(): checks that the sender wallet has sufficient balance for the requested amount.
// settle(): performs the balance transfer via EasyNetDb::transferValue.
class EasyNetFacilitatorClient : public FacilitatorClient {
public:
    explicit EasyNetFacilitatorClient( EthAddress& assetAddress, u256& chainId );


    nlohmann::json verify(const nlohmann::json& verifyRequestJson, EasyNetDb& db);

    nlohmann::json settle( const nlohmann::json& settlementRequestJson, EasyNetDb& db);

private:
    EthAddress assetAddress_;
    u256 chainId_;
    mutable std::shared_mutex mutex_;

    pair<ptr< PaymentPayload >, ptr<PaymentRequirements>> verifyUnsafe(const nlohmann::json& verifyRequestJson,
        optional< string >& error, EasyNetDb& db) const;

    // Extract required fields or throw with nested context.
    EthAddress parseAddressFromJson( const nlohmann::json& j, const std::string& key ) const;
    EthAddress getAssetAddress(
        const nlohmann::json& instruction, const nlohmann::json& payload ) const;



};
