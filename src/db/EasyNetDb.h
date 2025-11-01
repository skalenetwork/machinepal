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


    // Transfer value between two wallet addresses for a given asset.
    // Throws if sender wallet/asset pair does not exist or insufficient balance.
    void transferValue(const EthAddress &fromAddress, const EthAddress &toAddress, const EthAddress &assetAddress, const EIP3009Value &value);
private:
    mutable std::shared_mutex stateMutex_; // protects state table operations
};
