#include "MachinePayCommon.h"
#include "EIP3009ValidityTime.h"
#include <limits>

EIP3009ValidityTime::EIP3009ValidityTime(const u256& val) : time_(val) {}

std::string EIP3009ValidityTime::toDecimal() const {
    return time_.str();
}

EIP3009ValidityTime EIP3009ValidityTime::fromDecimal(const std::string& decStr) {
    u256 val(decStr);
    return EIP3009ValidityTime(val);
}

EIP3009ValidityTime EIP3009ValidityTime::fromTimeT(std::time_t timeT) {
    return EIP3009ValidityTime(u256(static_cast<unsigned long long>(timeT)));
}