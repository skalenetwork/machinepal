#include "MachinePayCommon.h"
#include "EIP3009Nonce.h"
#include "Hex.h"
#include <stdexcept>
#include <algorithm>
#include <random>
#undef random

EIP3009Nonce::EIP3009Nonce(const std::array<uint8_t, 32>& arr) : bytes_(arr) {}



EIP3009Nonce EIP3009Nonce::generateRandomNonce() {
    std::array<uint8_t, 32> arr;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 255);
    for (auto& b : arr) {
        b = static_cast<uint8_t>(dis(gen));
    }
    return EIP3009Nonce(arr);
}

const std::array<uint8_t, 32>& EIP3009Nonce::bytes() const {
    return bytes_;
}

std::string EIP3009Nonce::toHex(bool withPrefix) const {
    return Hex::toHex(std::span<const uint8_t>(bytes_.data(), bytes_.size()), withPrefix);
}

EIP3009Nonce EIP3009Nonce::fromHex(const std::string& hexStr) {
    auto vec = Hex::fromHex(hexStr);
    if (vec.size() > 32) throw std::invalid_argument("EIP3009Nonce must be at most 32 bytes");
    std::array<uint8_t, 32> arr{};
    // Pad with zeros on the left
    std::copy(vec.begin(), vec.end(), arr.begin() + (32 - vec.size()));
    return EIP3009Nonce(arr);
}
