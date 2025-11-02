#pragma once
#include <string>


class EIP3009Value
{
    u256 value_{};

public:
    [[nodiscard]] u256 value() const
    {
        return value_;
    }

    EIP3009Value() = default;
    explicit EIP3009Value(const u256& val);
    std::string toDecimal() const;
    std::string toDbString() const;
    static EIP3009Value fromHexOrDecimal(const std::string& decStr);
    friend auto operator<=>(const EIP3009Value&, const EIP3009Value&) = default;
};