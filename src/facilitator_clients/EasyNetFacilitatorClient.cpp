#include "EasyNetFacilitatorClient.h"
#include "MachinePayCommon.h"
#include "crypto/Encoding.h"
#include "payment/datastructures/PaymentRequirements.h"
#include "payment/datastructures/SettlementResponse.h"

#include <limits> // for numeric_limits<u256>::max()

EasyNetFacilitatorClient::EasyNetFacilitatorClient(EasyNetDb &db, EthAddress assetAddress,
                                                   u256 chainId)
    : db_(db),
      asetAddress_(std::move(assetAddress)),
      chainId_(chainId_) {
}


EthAddress EasyNetFacilitatorClient::getAssetAddress(const nlohmann::json &paymentReqs) const {
    auto paymentRequirements = PaymentRequirements::fromJson(paymentReqs);
    return EthAddress::parseFlexible(paymentRequirements->asset());
}

ptr<PaymentPayload>  EasyNetFacilitatorClient::verifyCore(const nlohmann::json &paymentPayloadJson, EthAddress &fromAddress, optional<string> &error) const
{
    auto paymentPayload = PaymentPayload::fromJson(paymentPayloadJson);
    auto paymentsRequirements = PaymentRequirements::fromJson(paymentReqs);

    fromAddress = paymentPayload->payload()->authorization()->from();
    EthAddress toAddress = paymentPayload->payload()->authorization()->to();
    EthAddress assetAddress = paymentsRequirements.assetAddress();
    EIP3009Value amountValue = paymentPayload->payload()->authorization()->value();

    SettlementResponse response = SettlementResponse::createValidResponse(
        fromAddress, toAddress, assetAddress, amountValue);

    u256 currentBalance = 1000000000 * u256(1000000000000000000ULL); // 1e27 initial funding for new wallets
    auto balanceOpt = db_.getBalance(fromAddress, assetAddress);
    if (balanceOpt.has_value()) {
        currentBalance = balanceOpt.value();
    }
    // Overflow check on receiver side (if we can read it) purely informational

    auto receiverBalanceOpt = db_.getBalance(toAddress, assetAddress);
    if (receiverBalanceOpt.has_value()) {
        const u256 maxVal = (std::numeric_limits<u256>::max)();
        u256 receiverBal = receiverBalanceOpt.value();
        if (requested > maxVal - receiverBal) {
            error = "Overflow:Receiver balance would overflow 256-bit limit";
            return paymentPayload;
        }
    }

    if (requested > currentBalance) {
        error = "InsufficientFunds : Balance lower than requested transfer amount";
        return paymentPayload;
    }

    return paymentPayload;
}

nlohmann::json EasyNetFacilitatorClient::verify(const nlohmann::json &paymentRequirementsJson,
                                                const nlohmann::json &paymentPayloadJson) const {
    try {
        EthAddress fromAddress;
        optional<string> error;
        verifyCore(paymentPayloadJson, fromAddress, error);
        if (error) {
            return nlohmann::json{{"valid", false},
                                  {"invalidReason", error.value()},
                                  {"payer", fromAddress.toDbString()}};
        } else {
            return nlohmann::json{
                {"valid", true},
                {"from", fromAddress.toDbString()}};
        }
    } catch ( const std::exception & e) {
        return nlohmann::json{{"valid", false}, {"invalidReason", e.what()}};
    }
}

nlohmann::json EasyNetFacilitatorClient::settle(const nlohmann::json &paymentInstruction,
                                                const nlohmann::json &paymentPayload) const {

    try {

        EthAddress fromAddress;
        optional<string> error;
        verifyCore(paymentPayloadJson, fromAddress, error);


        EthAddress fromAddress = parseAddressFromJson(paymentPayload, "from");
        EthAddress toAddress = parseAddressFromJson(paymentPayload, "to");
        EthAddress assetAddress = getAssetAddress(paymentInstruction, paymentPayload);

        EIP3009Value amountValue;

        amountValue = EIP3009Value::fromHexOrDecimal(paymentPayload["amount"].get<std::string>());

        // Optionally auto-fund sender if new (development convenience)
        db_.fundUserWalletWithFundsIfNewWallet(fromAddress, assetAddress);
        auto balanceBeforeOpt = db_.getBalance(fromAddress, assetAddress);
        u256 balanceBefore = balanceBeforeOpt.value_or(0);

        auto result = db_.transferValue(fromAddress, toAddress, assetAddress, amountValue);

        if (result == EasyNetDb::TransferResult::TransferSuccess) {
            auto balanceAfterOpt = db_.getBalance(fromAddress, assetAddress);
            u256 balanceAfter = balanceAfterOpt.value_or(0);
            return nlohmann::json{
                {"settled", true},
                {"status", "TransferSuccess"},
                {"from", fromAddress.toDbString()},
                {"to", toAddress.toDbString()},
                {"asset", assetAddress.toDbString()},
                {"amount", amountValue.toDbString()},
                {"balanceBefore", Encoding::u256ToDecimal(balanceBefore)},
                {"balanceAfter", Encoding::u256ToDecimal(balanceAfter)}
            };
        }

        return nlohmann::json{
            {"settled", false},
            {"status", "InsufficientFunds"},
            {"from", fromAddress.toDbString()},
            {"to", toAddress.toDbString()},
            {"asset", assetAddress.toDbString()},
            {"amount", amountValue.toDbString()},
            {"balanceBefore", Encoding::u256ToDecimal(balanceBefore)}
        };
    } catch (const std::exception &e) {
        return nlohmann::json{{"settled", false}, {"status", "Exception"}, {"message", e.what()}};
    }
}