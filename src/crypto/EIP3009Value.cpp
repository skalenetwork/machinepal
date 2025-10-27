#include "MachinePayCommon.h"
#include "EIP3009Value.h"

EIP3009Value::EIP3009Value(const u256& val) : value_(val) {}

std::string EIP3009Value::toDecimal() const {
    return value_.str();
}

EIP3009Value EIP3009Value::fromDecimal(const std::string& decStr) {
    u256 val(decStr);
    return EIP3009Value(val);
}

