#include "MachinePayCommon.h"
#include "EIP3009Value.h"

#include "Hex.h"

EIP3009Value::EIP3009Value(const u256& val) : value_(val) {}

std::string EIP3009Value::toDecimal() const {
    return Hex::u256ToDecimal(value_);
}

EIP3009Value EIP3009Value::fromHexOrDecimal(const std::string& decStr) {
    u256 val = Hex::fromHexOrDecimal(decStr);
    return EIP3009Value(val);
}

