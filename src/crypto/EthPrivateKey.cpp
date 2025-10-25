#include "EthPrivateKey.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdlib>

// Helper function to convert hex string to bytes
static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 2; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        bytes.push_back((uint8_t) strtol(byteString.c_str(), NULL, 16));
    }
    return bytes;
}

EthPrivateKey::EthPrivateKey() : bytes_{} {}

EthPrivateKey::EthPrivateKey(const std::array<uint8_t, 32>& bytes) : bytes_(bytes) {}

EthPrivateKey::EthPrivateKey(std::span<const uint8_t, 32> bytes) {
    std::copy(bytes.begin(), bytes.end(), bytes_.begin());
}

EthPrivateKey::EthPrivateKey(const uint8_t* data, std::size_t len) {
    if (len != 32) {
        throw std::invalid_argument("EthPrivateKey must be 32 bytes");
    }
    std::copy(data, data + len, bytes_.begin());
}

EthPrivateKey::EthPrivateKey(const std::string& hex) {
    if (hex.rfind("0x", 0) != 0 || hex.length() != 66) {
        throw std::invalid_argument("Invalid hex string for EthPrivateKey");
    }
    auto bytes = hexToBytes(hex);
    std::copy(bytes.begin(), bytes.end(), bytes_.begin());
}

EthPrivateKey EthPrivateKey::parseHex(const std::string& hex) {
    return EthPrivateKey(hex);
}

std::string EthPrivateKey::toHex() const {
    std::stringstream ss;
    ss << "0x";
    for (const auto& byte : bytes_) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

bool operator==(const EthPrivateKey& a, const EthPrivateKey& b) {
    return a.bytes_ == b.bytes_;
}

bool operator!=(const EthPrivateKey& a, const EthPrivateKey& b) {
    return !(a == b);
}
