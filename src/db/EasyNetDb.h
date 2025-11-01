#pragma once

#include <optional>
#include "MachinePayCommon.h"
#include "MachinePayDb.h"

class EasyNetDb : public MachinePayDb {
public:
    EasyNetDb(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo);
    // Add custom methods or overrides here if needed
};
