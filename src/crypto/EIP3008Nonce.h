#pragma once
#include <array>
#include <string>

class EIP3008Nonce {
    std::array<uint8_t, 32> bytes_;
public:
    // Construct from vector
    explicit EIP3008Nonce(const std::array<uint8_t, 32>& arr);
    // Construct from hex string (with or without 0x)
    static EIP3008Nonce fromHex(const std::string& hexStr);
    // Generate a random 32-byte nonce
    static EIP3008Nonce generateRandomNonce();
    const std::array<uint8_t, 32>& bytes() const;
    std::string toHex(bool withPrefix = false) const;

    friend auto operator<=>(const EIP3008Nonce&, const EIP3008Nonce&) = default;

};