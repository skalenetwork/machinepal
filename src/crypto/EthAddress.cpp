//
// Created by kladko on 10/24/25.
//
#include "MachinePayCommon.h"

#include "EthAddress.h"
#include "Keccak.h"

#include <boost/algorithm/hex.hpp>
#include <algorithm>
#include <cctype>
#include <span>
#include <stdexcept>

EthAddress EthAddress::parseHexAddress(const std::string& hex) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() != 40) {
        throw std::invalid_argument("Address hex must be 40 characters (20 bytes)");
    }
    EthAddress addr{};
    try {
        // decode into addr; boost::algorithm::unhex throws hex_decode_error on invalid input
        boost::algorithm::unhex(s.begin(), s.end(), addr.bytes().begin());
    } catch (const boost::algorithm::hex_decode_error& e) {
        throw std::invalid_argument(std::string("Invalid hex character in address: ") + e.what());
    }
    return addr;
}

EthAddress EthAddress::parseFlexible(const std::string &hex, bool validateChecksum) {
    // Accept optional 0x prefix, lowercase or checksum form.
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) s = s.substr(2);
    if (s.size() != 40) throw std::invalid_argument("Address hex must be 40 characters (20 bytes)");
    // Validate characters
    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            throw std::invalid_argument("Invalid hex character in address");
        }
    }
    // If mixed case and validateChecksum, verify EIP-55
    bool hasUpper = false, hasLower = false;
    for (char c : s) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            if (std::islower(static_cast<unsigned char>(c))) hasLower = true; else hasUpper = true;
        }
    }
    bool mixed = hasUpper && hasLower;
    std::string lower;
    lower.reserve(40);
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    EthAddress addr = parseHexAddress(lower); // reuse decoder
    if (validateChecksum && mixed) {
        std::string expected = addr.toChecksumHex().substr(2); // remove 0x
        if (expected != s) {
            throw std::invalid_argument("Checksum mismatch for address");
        }
    }
    return addr;
}

std::string EthAddress::toHex() const {
    std::string out;
    out.reserve(42);
    out += "0x";
    boost::algorithm::hex_lower(bytes().begin(), bytes().end(), std::back_inserter(out));
    return out;
}

std::string EthAddress::toChecksumHex() const {
    // EIP-55 checksum
    std::string lower;
    lower.reserve(40);
    for (auto b : bytes_) {
        const char hexDigits[] = "0123456789abcdef";
        lower.push_back(hexDigits[(b >> 4) & 0xF]);
        lower.push_back(hexDigits[b & 0xF]);
    }
    auto hashArr = keccak::keccak256(lower); // hash of lowercase hex string (no 0x)
    std::string out = "0x";
    out.reserve(42);
    for (size_t i = 0; i < lower.size(); ++i) {
        char c = lower[i];
        if (std::isalpha(static_cast<unsigned char>(c))) {
            // Determine nibble from hash
            uint8_t byte = hashArr[i / 2];
            uint8_t nibble = (i % 2 == 0) ? (byte >> 4) & 0xF : byte & 0xF;
            out.push_back(nibble >= 8 ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c);
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// Constructors
EthAddress::EthAddress() = default;
EthAddress::EthAddress(const std::array<uint8_t, 20>& bytes) : bytes_(bytes) {}
EthAddress::EthAddress(std::span<const uint8_t, 20> bytes) { std::copy(bytes.begin(), bytes.end(), bytes_.begin()); }
EthAddress::EthAddress(const uint8_t* data, std::size_t len) { if (len != 20) throw std::invalid_argument("Address length must be 20 bytes"); std::copy(data, data + 20, bytes_.begin()); }
EthAddress::EthAddress(const std::string& hex) { *this = parseHexAddress(hex); }

// Friend operators
bool operator==(const EthAddress& a, const EthAddress& b) { return a.bytes_ == b.bytes_; }
bool operator!=(const EthAddress& a, const EthAddress& b) { return !(a == b); }
bool operator<(const EthAddress& a, const EthAddress& b) { return std::lexicographical_compare(a.bytes_.begin(), a.bytes_.end(), b.bytes_.begin(), b.bytes_.end()); }
