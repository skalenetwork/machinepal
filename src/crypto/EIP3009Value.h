#pragma once
#include <string>
#include <ostream>


class EIP3009Value {
    u256 value_{};

public:
    [[nodiscard]] u256 value() const { return value_; }

    EIP3009Value() = default;
    explicit EIP3009Value( const u256& val );
    std::string toDecimal() const;
    std::string toDbString() const;
    static EIP3009Value fromHexOrDecimal( const std::string& decStr );
    friend bool operator==(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return a.value_ == b.value_;
    }
    friend bool operator!=(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return a.value_ < b.value_;
    }
    friend bool operator>(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return b < a;
    }
    friend bool operator<=(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return !(b < a);
    }
    friend bool operator>=(const EIP3009Value& a, const EIP3009Value& b) noexcept {
        return !(a < b);
    }
    // For Boost.Test diagnostics and general logging/printing.
    friend std::ostream& operator<<(std::ostream& os, const EIP3009Value& v) {
        return os << v.toDecimal();
    }
};