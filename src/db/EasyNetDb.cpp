#include "MachinePayCommon.h"
#include "EasyNetDb.h"

EasyNetDb::EasyNetDb(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo)
: MachinePayDb(app, type, connectionInfo) {
    // Custom initialization for EasyNetDb if needed
}
