#pragma once

#include <optional>
#include <shared_mutex> // added for read/write locking
#include "MachinePayCommon.h"
#include "MachinePayDb.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP3009Value.h"

class EasyNetDb : public MachinePayDb {
public:
    EasyNetDb(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo);


     void newWallet(const EthAddress &walletAddress, const EthAddress &assetAddress, const EIP3009Value &value);

    enum class TransferResult { TransferSuccess, InsufficientFunds };
    TransferResult processTransferRequest(const EthAddress &fromAddress, const EthAddress &toAddress,
                                          const EthAddress &assetAddress,
                                          const EIP3009Value &value);
    TransferResult transferValue(const EthAddress &fromAddress, const EthAddress &toAddress, const EthAddress &assetAddress, const EIP3009Value &value);
    // Funds a wallet with initial tokens if it does not yet exist for the given asset.
    // Initial amount: 1,000,000,000 * 10^18 (1e27) token units.
    void fundUserWalletWithFundsIfNewWallet(const EthAddress &walletAddress, const EthAddress &assetAddress);
private:
    mutable std::shared_mutex stateMutex_; // protects state table operations
};
