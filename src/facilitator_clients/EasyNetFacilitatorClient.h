#pragma once

#include "FacilitatorClient.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EthAddress.h"
#include "db/EasyNetDb.h"
#include <optional>

class PaymentRequirements;
// A local in-process facilitator implementation that uses EasyNetDb instead of
// making remote HTTP calls. Intended for development, testing, or offline mode.
//
// verify(): checks that the sender wallet has sufficient balance for the requested amount.
// settle(): performs the balance transfer via EasyNetDb::transferValue.
class EasyNetFacilitatorClient : public FacilitatorClient {
public:
    explicit EasyNetFacilitatorClient( EasyNetDb& db, EthAddress& assetAddress, u256& chainId );
    pair<ptr< PaymentPayload >, ptr<PaymentRequirements>> verifyUnsafe(const nlohmann::json& verifyRequestJson,
        optional< string >& error) const;

    nlohmann::json verify(
        const nlohmann::json& verifyRequestJson) const override;

    nlohmann::json settle( const nlohmann::json& paymentInstruction,
        const nlohmann::json& paymentPayload ) const override;

private:
    EasyNetDb& db_;
    EthAddress assetAddress_;
    u256 chainId_;
    mutable std::shared_mutex mutex_;

    // Extract required fields or throw with nested context.
    EthAddress parseAddressFromJson( const nlohmann::json& j, const std::string& key ) const;
    EthAddress getAssetAddress(
        const nlohmann::json& instruction, const nlohmann::json& payload ) const;



};
