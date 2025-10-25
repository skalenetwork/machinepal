//
// Created by kladko on 10/24/25.
//
#include "MachinePayCommon.h"

#include "Address.h"

#include <boost/algorithm/hex.hpp>
#include <algorithm>
#include <span>
#include <stdexcept>

Address Address::parseHexAddress(const std::string& hex) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() != 40) {
        throw std::invalid_argument("Address hex must be 40 characters (20 bytes)");
    }
    Address addr{};
    try {
        // decode into addr; boost::algorithm::unhex throws hex_decode_error on invalid input
        boost::algorithm::unhex(s.begin(), s.end(), addr.bytes().begin());
    } catch (const boost::algorithm::hex_decode_error& e) {
        throw std::invalid_argument(std::string("Invalid hex character in address: ") + e.what());
    }
    return addr;
}

std::string Address::toHex() const {
    std::string out;
    out.reserve(42);
    out += "0x";
    boost::algorithm::hex_lower(bytes().begin(), bytes().end(), std::back_inserter(out));
    return out;
}

// Constructors
Address::Address() = default;
Address::Address(const std::array<uint8_t, 20>& bytes) : bytes_(bytes) {}
Address::Address(std::span<const uint8_t, 20> bytes) { std::copy(bytes.begin(), bytes.end(), bytes_.begin()); }
Address::Address(const uint8_t* data, std::size_t len) { if (len != 20) throw std::invalid_argument("Address length must be 20 bytes"); std::copy(data, data + 20, bytes_.begin()); }
Address::Address(const std::string& hex) { *this = parseHexAddress(hex); }

// Friend operators
bool operator==(const Address& a, const Address& b) { return a.bytes_ == b.bytes_; }
bool operator!=(const Address& a, const Address& b) { return !(a == b); }
bool operator<(const Address& a, const Address& b) { return std::lexicographical_compare(a.bytes_.begin(), a.bytes_.end(), b.bytes_.begin(), b.bytes_.end()); }
