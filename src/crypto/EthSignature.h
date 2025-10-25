#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <span>
#include <vector>

class EthSignature {
public:
    EthSignature();
    explicit EthSignature(const std::array<uint8_t, 65> &bytes); // Ethereum signatures are 65 bytes (r,s,v)
    explicit EthSignature(std::span<const uint8_t, 65> bytes);
    EthSignature(const uint8_t *data, std::size_t len);
    explicit EthSignature(const std::string &hex);
    EthSignature(const std::array<uint8_t, 32>& r, const std::array<uint8_t, 32>& s, uint8_t v);

    // Accessors for r, s, v
    std::array<uint8_t, 32> r() const;
    std::array<uint8_t, 32> s() const;
    uint8_t v() const;

    [[nodiscard]] std::array<uint8_t, 65> &bytes() { return bytes_; }
    [[nodiscard]] const std::array<uint8_t, 65> &bytes() const { return bytes_; }
    std::string toHex(bool withPrefix = false) const;

    static EthSignature parseHex(const std::string &hex);
    static EthSignature parseFlexible(const std::string &hex); // optional 0x prefix


    friend bool operator==(const EthSignature &a, const EthSignature &b);
    friend bool operator!=(const EthSignature &a, const EthSignature &b);

    ~EthSignature();

    // Validity check for Ethereum/Hardhat signature
    static bool isValid(const std::array<uint8_t,65>& sigBytes);

private:
    std::array<uint8_t, 65> bytes_{};
    static bool isValidV(uint8_t v);
};
