#include "EthSignature.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>

EthSignature::EthSignature() = default;

EthSignature::EthSignature(const std::array<uint8_t, 65> &bytes) : bytes_(bytes) {}

EthSignature::EthSignature(std::span<const uint8_t, 65> bytes) {
    std::copy(bytes.begin(), bytes.end(), bytes_.begin());
}

EthSignature::EthSignature(const uint8_t *data, std::size_t len) {
    if (len != 65) throw std::invalid_argument("Signature must be 65 bytes");
    std::copy(data, data + 65, bytes_.begin());
}

EthSignature::EthSignature(const std::string &hex) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        s = s.substr(2);
    if (s.size() != 130) throw std::invalid_argument("Hex string must be 130 chars for 65 bytes");
    boost::algorithm::unhex(s.begin(), s.end(), bytes_.begin());
}

EthSignature::EthSignature(const std::array<uint8_t, 32>& r, const std::array<uint8_t, 32>& s, uint8_t v) {
    std::copy(r.begin(), r.end(), bytes_.begin());
    std::copy(s.begin(), s.end(), bytes_.begin() + 32);
    bytes_[64] = v;
}

std::array<uint8_t, 32> EthSignature::r() const {
    std::array<uint8_t, 32> r;
    std::copy(bytes_.begin(), bytes_.begin() + 32, r.begin());
    return r;
}
std::array<uint8_t, 32> EthSignature::s() const {
    std::array<uint8_t, 32> s;
    std::copy(bytes_.begin() + 32, bytes_.begin() + 64, s.begin());
    return s;
}
uint8_t EthSignature::v() const {
    return bytes_[64];
}

std::string EthSignature::toHex(bool withPrefix) const {
    std::string hex;
    boost::algorithm::hex(bytes_.begin(), bytes_.end(), std::back_inserter(hex));
    if (withPrefix) return "0x" + hex;
    return hex;
}

EthSignature EthSignature::parseHex(const std::string &hex) {
    return EthSignature(hex);
}

EthSignature EthSignature::parseFlexible(const std::string &hex) {
    return EthSignature(hex);
}

bool operator==(const EthSignature &a, const EthSignature &b) {
    return a.bytes_ == b.bytes_;
}

bool operator!=(const EthSignature &a, const EthSignature &b) {
    return !(a == b);
}

EthSignature::~EthSignature() = default;



bool EthSignature::isValid(const std::array<uint8_t,65>& sigBytes) {
    uint8_t v = sigBytes[64];
    return isValidV(v);
}

bool EthSignature::isValidV(uint8_t v) {
    return v == 27 || v == 28 || v == 0 || v == 1;
}
