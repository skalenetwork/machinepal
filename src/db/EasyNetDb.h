#pragma once

#include <optional>
#include "MachinePayCommon.h"
#include "MachinePayDb.h"

class EasyNetDb : public MachinePayDb {
public:
    EasyNetDb(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo);


     void newWallet(const EthAddress &walletAddress, const EthAddress &assetAddress, const EIP3009Value &value);


};
