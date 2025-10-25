#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <algorithm>
#include <span>

class Address {
public:
    // Standard constructors (implemented in Address.cpp)
    Address();
    explicit Address(const std::array<uint8_t, 20> &bytes);
    explicit Address(std::span<const uint8_t, 20> bytes);
    Address(const uint8_t *data, std::size_t len);
    explicit Address(const std::string &hex);

    // Mutable and const accessors (kept inline)
    [[nodiscard]] std::array<uint8_t, 20> &bytes() { return bytes_; }
    [[nodiscard]] const std::array<uint8_t, 20> &bytes() const { return bytes_; }

    static Address parseHexAddress(const std::string &hex);
    std::string toHex() const;

    // Comparison operators (defined in Address.cpp)
    friend bool operator==(const Address &a, const Address &b);
    friend bool operator!=(const Address &a, const Address &b);
    friend bool operator<(const Address &a, const Address &b);

private:
    std::array<uint8_t, 20> bytes_{};
};
