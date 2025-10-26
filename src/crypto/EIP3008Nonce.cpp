#include "MachinePayCommon.h"
#include "EIP3008Nonce.h"
#include "Hex.h"
#include <stdexcept>
#include <algorithm>
#include <random>
#undef random

EIP3008Nonce::EIP3008Nonce(const std::array<uint8_t, 32>& arr) : bytes_(arr) {}

EIP3008Nonce EIP3008Nonce::fromHex(const std::string& hexStr) {
    auto vec = Hex::fromHex(hexStr);
    if (vec.size() != 32) throw std::invalid_argument("EIP3008Nonce must be 32 bytes");
    std::array<uint8_t, 32> arr;
    std::copy(vec.begin(), vec.end(), arr.begin());
    return EIP3008Nonce(arr);
}

EIP3008Nonce EIP3008Nonce::generateRandomNonce() {
    std::array<uint8_t, 32> arr;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 255);
    for (auto& b : arr) {
        b = static_cast<uint8_t>(dis(gen));
    }
    return EIP3008Nonce(arr);
}

const std::array<uint8_t, 32>& EIP3008Nonce::bytes() const {
    return bytes_;
}

std::string EIP3008Nonce::toHex(bool withPrefix) const {
    return Hex::toHex(std::span<const uint8_t>(bytes_.data(), bytes_.size()), withPrefix);
}
