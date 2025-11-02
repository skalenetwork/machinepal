#include "EasyNetFacilitatorClient.h"
#include "MachinePayCommon.h"
#include "crypto/Encoding.h"
#include "payment/datastructures/PaymentRequirements.h"
#include "payment/datastructures/SettlementResponse.h"

#include <limits>  // for numeric_limits<u256>::max()

EasyNetFacilitatorClient::EasyNetFacilitatorClient(
    EasyNetDb& database, EthAddress& assetAddress, u256& chainId )
    : db_( database ),
      assetAddress_( assetAddress ),
      chainId_( chainId ) {
}

ptr< PaymentPayload > EasyNetFacilitatorClient::verifyUnsafe(
    const nlohmann::json& paymentPayloadJson, const nlohmann::json& paymentRequirementsJson,
    EthAddress& fromWalletAddress, optional< string >& error ) const {
    auto paymentPayload = PaymentPayload::fromJson( paymentPayloadJson );
    auto paymentRequirements = PaymentRequirements::fromJson( paymentRequirementsJson );

    fromWalletAddress = paymentPayload->payload()->authorization()->from();
    EthAddress toWalletAddress = paymentPayload->payload()->authorization()->to();
    EthAddress assetWalletAddress = EthAddress::parseFlexible( paymentRequirements->asset() );
    EIP3009Value transferValue = paymentPayload->payload()->authorization()->value();

    u256 currentBalance = 1000000000 * u256( 1000000000000000000ULL );
    // 1e27 initial funding for new wallets
    auto senderBalanceOpt = db_.getBalance( fromWalletAddress, assetWalletAddress );
    if (senderBalanceOpt.has_value()) {
        currentBalance = senderBalanceOpt.value();
    }

    // Overflow check on receiver side (if we can read it) purely informational
    auto receiverBalanceOpt = db_.getBalance( toWalletAddress, assetWalletAddress );
    if (receiverBalanceOpt.has_value()) {
        const u256 maxVal = ( std::numeric_limits< u256 >::max )();
        u256 receiverBalance = receiverBalanceOpt.value();
        if (transferValue.value() > maxVal - receiverBalance) {
            error = "Overflow: Receiver balance would overflow 256-bit limit";
            return paymentPayload;
        }
    }

    if (transferValue.value() > currentBalance) {
        error = "InsufficientFunds: Balance lower than requested transfer amount";
        return paymentPayload;
    }

    return paymentPayload;
}

nlohmann::json EasyNetFacilitatorClient::verify( const nlohmann::json& paymentRequirementsJson,
    const nlohmann::json& paymentPayloadJson ) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    try {
        EthAddress fromWalletAddress;
        optional< string > error;
        auto payload =
            verifyUnsafe( paymentPayloadJson, paymentRequirementsJson, fromWalletAddress, error );
        if (error) {
            return nlohmann::json{ { "valid", false }, { "invalidReason", error.value() },
                                   { "payer", fromWalletAddress.toDbString() } };
        } else {
            return nlohmann::json{ { "valid", true }, { "from", fromWalletAddress.toDbString() } };
        }
    } catch (const std::exception& e) {
        return nlohmann::json{ { "valid", false }, { "invalidReason", e.what() } };
    }
}

nlohmann::json EasyNetFacilitatorClient::settle(
    const nlohmann::json& paymentRequirementsJson,
    const nlohmann::json& paymentPayloadJson ) const
{

    std::unique_lock<std::shared_mutex> lock(mutex_);
    try {
        EthAddress fromWalletAddress;
        optional< string > error;
        auto paymentPayload =
            verifyUnsafe( paymentPayloadJson, paymentRequirementsJson, fromWalletAddress, error );

        if (error) {
            return nlohmann::json{ { "valid", false }, { "invalidReason", error.value() },
                                   { "payer", fromWalletAddress.toDbString() },
                                   { "transaction", "" } };
        }

        EthAddress toWalletAddress = paymentPayload->payload()->authorization()->to();
        EthAddress assetWalletAddress = EthAddress::parseFlexible(
            PaymentRequirements::fromJson( paymentRequirementsJson )->asset() );
        EIP3009Value transferValue = paymentPayload->payload()->authorization()->value();

        auto result = db_.processTransferRequest(
            fromWalletAddress, toWalletAddress, assetWalletAddress, transferValue );

        if (result == EasyNetDb::TransferResult::TransferSuccess) {
            return nlohmann::json{
                { "valid", true },
                { "from", fromWalletAddress.toDbString() },
                { "transaction", "" }
            };
        } else {
            return nlohmann::json{ { "valid", false }, { "from", fromWalletAddress.toDbString() },
                { "transaction", "" }, { "status", "InsufficientFunds" } };
        }
    } catch (const std::exception& e) {
        return nlohmann::json{ { "settled", false }, { "status", "Exception" },
            { "message", e.what() } };
    }
}
