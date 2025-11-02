#include "EasyNetFacilitatorClient.h"
#include "MachinePayCommon.h"
#include "crypto/Encoding.h"
#include "payment/datastructures/PaymentRequirements.h"

#include <limits> // for numeric_limits<u256>::max()

EasyNetFacilitatorClient::EasyNetFacilitatorClient(EasyNetDb &db, EthAddress defaultAssetAddress)
    : db_(db), defaultAssetAddress_(std::move(defaultAssetAddress)) {}




EthAddress EasyNetFacilitatorClient::getAssetAddress(const nlohmann::json &paymentReqs) const {
    auto paymentRequirements = PaymentRequirements::fromJson(paymentReqs);
    return EthAddress::parseFlexible(paymentRequirements->asset());
}

nlohmann::json EasyNetFacilitatorClient::verify(const nlohmann::json &paymentInstruction,
                                                const nlohmann::json &paymentPayload) const {
    try {
        // Extract addresses
        EthAddress fromAddress = parseAddressFromJson(paymentPayload, "from");
        EthAddress toAddress = parseAddressFromJson(paymentPayload, "to");
        EthAddress assetAddress = getAssetAddress(paymentInstruction, paymentPayload);

        // Extract amount (prefer payload.amount then instruction.amount)
        EIP3009Value amountValue;
        if (paymentPayload.contains("amount") && paymentPayload["amount"].is_string()) {
            amountValue = EIP3009Value::fromHexOrDecimal(paymentPayload["amount"].get<std::string>());
        } else if (paymentInstruction.contains("amount") && paymentInstruction["amount"].is_string()) {
            amountValue = EIP3009Value::fromHexOrDecimal(paymentInstruction["amount"].get<std::string>());
        } else {
            RETHROW_NESTED2("Missing amount field in paymentPayload or paymentInstruction");
        }

        // Fast-path no-op checks
        if (fromAddress.toDbString() == toAddress.toDbString() || amountValue.value() == 0) {
            return nlohmann::json{
                {"valid", true},
                {"noOp", true},
                {"reason", "Source and destination identical or zero amount"},
                {"amount", amountValue.toDbString()}
            };
        }

        // Read balance
        auto balanceOpt = db_.getBalance(fromAddress, assetAddress);
        if (!balanceOpt.has_value()) {
            return nlohmann::json{
                {"valid", false},
                {"error", "SenderWalletNotFound"},
                {"reason", "No state row for sender wallet & asset"}
            };
        }
        u256 currentBalance = balanceOpt.value();
        u256 requested = amountValue.value();

        // Overflow check on receiver side (if we can read it) purely informational
        auto receiverBalanceOpt = db_.getBalance(toAddress, assetAddress);
        bool wouldOverflow = false;
        if (receiverBalanceOpt.has_value()) {
            const u256 maxVal = (std::numeric_limits<u256>::max)();
            u256 receiverBal = receiverBalanceOpt.value();
            if (requested > maxVal - receiverBal) {
                wouldOverflow = true;
            }
        }

        if (requested > currentBalance) {
            return nlohmann::json{
                {"valid", false},
                {"error", "InsufficientFunds"},
                {"reason", "Balance lower than requested transfer amount"},
                {"have", Encoding::u256ToDecimal(currentBalance)},
                {"need", Encoding::u256ToDecimal(requested)}
            };
        }
        if (wouldOverflow) {
            return nlohmann::json{
                {"valid", false},
                {"error", "Overflow"},
                {"reason", "Receiver balance would overflow 256-bit limit"}
            };
        }

        return nlohmann::json{
            {"valid", true},
            {"from", fromAddress.toDbString()},
            {"to", toAddress.toDbString()},
            {"asset", assetAddress.toDbString()},
            {"amount", amountValue.toDbString()},
            {"balanceBefore", Encoding::u256ToDecimal(currentBalance)}
        };
    } catch (const std::exception &e) {
        return nlohmann::json{{"valid", false}, {"error", "Exception"}, {"message", e.what()}};
    }
}

nlohmann::json EasyNetFacilitatorClient::settle(const nlohmann::json &paymentInstruction,
                                                const nlohmann::json &paymentPayload) const {
    try {
        EthAddress fromAddress = parseAddressFromJson(paymentPayload, "from");
        EthAddress toAddress = parseAddressFromJson(paymentPayload, "to");
        EthAddress assetAddress = getAssetAddress(paymentInstruction, paymentPayload);

        EIP3009Value amountValue;
        if (paymentPayload.contains("amount") && paymentPayload["amount"].is_string()) {
            amountValue = EIP3009Value::fromHexOrDecimal(paymentPayload["amount"].get<std::string>());
        } else if (paymentInstruction.contains("amount") && paymentInstruction["amount"].is_string()) {
            amountValue = EIP3009Value::fromHexOrDecimal(paymentInstruction["amount"].get<std::string>());
        } else {
            RETHROW_NESTED2("Missing amount field for settlement");
        }

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
