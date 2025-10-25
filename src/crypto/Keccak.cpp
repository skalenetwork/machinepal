#include "Keccak.h"
#include <cryptopp/keccak.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <span>

namespace keccak {

std::array<uint8_t, 32> keccak256(std::span<const uint8_t> data) {
    std::array<uint8_t, 32> out{};
    CryptoPP::Keccak_256 hash;
    hash.Update(data.data(), data.size());
    hash.TruncatedFinal(out.data(), out.size());
    return out;
}

std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data) {
    return keccak256(std::span<const uint8_t>(data.data(), data.size()));
}

std::array<uint8_t, 32> keccak256(const std::string& ascii) {
    return keccak256(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ascii.data()), ascii.size()));
}

std::string keccak256Hex(std::span<const uint8_t> data) {
    auto h = keccak256(data);
    std::ostringstream oss; oss << "0x";
    for (auto b : h) {
        oss << std::hex << std::nouppercase;
        oss.width(2); oss.fill('0');
        oss << (int)b;
    }
    return oss.str();
}

std::string keccak256Hex(const std::string& ascii) {
    return keccak256Hex(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ascii.data()), ascii.size()));
}

} // namespace keccak
