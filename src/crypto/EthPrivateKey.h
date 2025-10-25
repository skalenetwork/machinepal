#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <algorithm>
#include <span>

class EthPrivateKey {
public:
    // Constructors
    EthPrivateKey();
    explicit EthPrivateKey(const std::array<uint8_t, 32> &bytes);
    explicit EthPrivateKey(std::span<const uint8_t, 32> bytes);
    EthPrivateKey(const uint8_t *data, std::size_t len);
    explicit EthPrivateKey(const std::string &hex);

    // Accessors
    [[nodiscard]] std::array<uint8_t, 32> &bytes() { return bytes_; }
    [[nodiscard]] const std::array<uint8_t, 32> &bytes() const { return bytes_; }

    static EthPrivateKey parseHex(const std::string &hex);
    static EthPrivateKey parseFlexible(const std::string &hex); // optional 0x prefix
    std::string toHex() const;

    // Comparison operators
    friend bool operator==(const EthPrivateKey &a, const EthPrivateKey &b);
    friend bool operator!=(const EthPrivateKey &a, const EthPrivateKey &b);

    ~EthPrivateKey();

private:
    std::array<uint8_t, 32> bytes_{};

    static bool isValidRange(const std::array<uint8_t,32>& k);
};
