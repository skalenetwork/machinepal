#include "EasyNetFacilitator.h"

#include "FacilitatorErrors.h"
#include "MachinePayApp.h"
#include "MachinePayCommon.h"

#include "payment/datastructures/PaymentRequirements.h"
#include "payment/datastructures/VerifyResponse.h"

EasyNetFacilitator::EasyNetFacilitator( MachinePayApp& app ) : app_( app ) {
    chainId_ = EIP712Domain::machinePayEasyNet()->chainId();
};

nlohmann::json EasyNetFacilitator::processSettleRequest(
    const nlohmann::json& settlementRequestJson ) {
    std::unique_lock< std::shared_mutex > lock( mutex_ );
    auto db = dynamic_pointer_cast< EasyNetDb >( app_.machinePayDB() );
    CHECK_STATE( db );
    try {
        optional< string > error;
        auto [paymentPayload, paymentReqs] =
            processVerifyRequestUnsafe( settlementRequestJson, error, *db );

        // Obtain from address after verification (available even if error)
        EthAddress fromWalletAddress = paymentPayload->payload()->authorization()->from();

        if ( error ) {
            SettlementResponse response( false, error.value(), "",
                "",  // transaction, network (none here)
                fromWalletAddress.toHex( PREFIX_0x ), std::nullopt );
            return response.toJson();
        }

        EthAddress toWalletAddress = paymentPayload->payload()->authorization()->to();
        EthAddress assetWalletAddress = EthAddress::parseFlexible( paymentReqs->asset() );
        EIP3009Value transferValue = paymentPayload->payload()->authorization()->value();
        EIP3009Nonce nonce = paymentPayload->payload()->authorization()->nonce();
        const string& resource = paymentReqs->resource();

        // Prepare extra parameters for updated EasyNetDb API
        std::string jsonInfo = paymentPayload->toJson().dump();
        std::string transactionHash =
            paymentPayload->payload()->signature().toHex( PREFIX_0x );
        std::string authorizationSignatureHash = Encoding::hashToHex(
            paymentPayload->payload()->signature().computeSignatureHash() );

        auto result = db->processTransferRequest( fromWalletAddress, toWalletAddress,
            assetWalletAddress, transferValue, nonce, resource, "0.0.0.0", jsonInfo,
            transactionHash, chainId_, authorizationSignatureHash );

        if ( result == EasyNetDb::TransferResult::TransferSuccess ) {
            SettlementResponse response( true, std::nullopt, "",
                "",  // transaction, network (empty placeholders)
                fromWalletAddress.toHex( PREFIX_0x ), std::nullopt );
            return response.toJson();
        } else {
            SettlementResponse response( false, std::string( "InsufficientFunds" ), "",
                "",  // transaction, network
                fromWalletAddress.toHex( PREFIX_0x ), std::nullopt );
            return response.toJson();
        }
    } catch ( const std::exception& e ) {
        SettlementResponse response( false, std::string( e.what() ), "",
            "",  // transaction, network
            "", std::nullopt );
        return response.toJson();
    }
}


pair< ptr< PaymentPayload >, ptr< PaymentRequirements > >
EasyNetFacilitator::processVerifyRequestUnsafe( const nlohmann::json& verifyRequestJson,
    optional< string >& error, EasyNetDb& db ) const {
    auto verifyRequest = SettlementRequest::fromJson( verifyRequestJson );
    auto paymentPayload = verifyRequest.paymentPayload();
    auto paymentRequirements = verifyRequest.paymentRequirements();
    auto fromWalletAddress = paymentPayload->payload()->authorization()->from();
    auto toWalletAddress = paymentPayload->payload()->authorization()->to();
    auto assetWalletAddress = EthAddress::parseFlexible( paymentRequirements->asset() );
    auto transferValue = paymentPayload->payload()->authorization()->value();

    auto payToAddress = EthAddress::parseFlexible(paymentRequirements->payTo());

    auto price = EIP3009Value::fromHexOrDecimal(paymentRequirements->maxAmountRequired());

    auto httpError = paymentPayload->validateAndVerifySignature( *app_.configManager()->latestConfig(),
        price, payToAddress, paymentRequirements->scheme());

    if ( httpError ) {
        spdlog::error("InvalidPayload: Payment payload validation or signature failed");
        error = FacilitatorErrors::getErrorString(FacilitatorError::invalid_payload );
        return { paymentPayload, paymentRequirements };
    }


    u256 currentBalance = 1000000000 * u256( 1000000000000000000ULL );
    // 1e27 initial funding for new wallets
    auto senderBalanceOpt = db.getBalance( fromWalletAddress, assetWalletAddress );
    if ( senderBalanceOpt.has_value() ) {
        currentBalance = senderBalanceOpt.value();
    }

    // Overflow check on receiver side (if we can read it) purely informational
    auto receiverBalanceOpt = db.getBalance( toWalletAddress, assetWalletAddress );
    if ( receiverBalanceOpt.has_value() ) {
        const u256 maxVal = ( std::numeric_limits< u256 >::max )();
        const u256 receiverBalance = receiverBalanceOpt.value();
        if ( transferValue.value() > maxVal - receiverBalance ) {
            spdlog::error("Overflow: Receiver balance would overflow 256-bit limit");
            error = FacilitatorErrors::getErrorString(FacilitatorError::insufficient_funds );
            return { paymentPayload, paymentRequirements };
        }
    }

    if ( transferValue.value() > currentBalance ) {
        spdlog::error("InsufficientFunds: Balance lower than requested transfer amount");
        error = FacilitatorErrors::getErrorString(FacilitatorError::insufficient_funds );
        return { paymentPayload, paymentRequirements };
    }

    return { paymentPayload, paymentRequirements };
}

nlohmann::json EasyNetFacilitator::processVerifyRequest(
    const nlohmann::json& verifyRequestJson ) {
    auto db = dynamic_pointer_cast< EasyNetDb >( app_.machinePayDB() );
    CHECK_STATE( db );
    std::shared_lock< std::shared_mutex > lock( mutex_ );
    try {
        optional< string > error;
        auto [payload, paymentReqs] =
            processVerifyRequestUnsafe( verifyRequestJson, error, *db );
        auto fromWalletAddress = payload->payload()->authorization()->from();
        if ( error ) {
            spdlog::error("Error processing verify request: {}", error.value());
            VerifyResponse errorResponse(
                false, error.value(), fromWalletAddress.toDbString(), std::nullopt );

            return errorResponse.toJson();
        } else {
            VerifyResponse verifyResponse(
                true, std::nullopt, fromWalletAddress.toDbString(), std::nullopt );
            return verifyResponse.toJson();
        }
    } catch ( const std::exception& e ) {
        spdlog::error("Exception processing verify request: {}", e.what());
        optional<string> standardErrorString =
            std::string(FacilitatorErrors::getErrorString(FacilitatorError::unexpected_verify_error));
        VerifyResponse errorResponse( false,
            standardErrorString, "", std::nullopt );
        return errorResponse.toJson();
    }
}