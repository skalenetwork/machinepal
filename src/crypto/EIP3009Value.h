#pragma once
#include <string>
#include <boost/multiprecision/cpp_int.hpp>

using u256 = boost::multiprecision::uint256_t;

class EIP3009Value {
    u256 value_{};

public:
    [[nodiscard]] u256 value() const {
        return value_;
    }

    EIP3009Value() = default;
    explicit EIP3009Value(const u256& val);
    std::string toDecimal() const;
    static EIP3009Value fromDecimal(const std::string& decStr);
    friend auto operator<=>(const EIP3009Value&, const EIP3009Value&) = default;
};

