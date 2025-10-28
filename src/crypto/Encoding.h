#pragma once
#include "boost/algorithm/hex.hpp"
#include "boost/algorithm/string/case_conv.hpp"
#include <boost/multiprecision/cpp_int.hpp>

#include <iostream>
#include <span>
#include <vector>

class Encoding {
public:
    static std::string toHex(const boost::multiprecision::uint256_t &val, bool withPrefix = false) {
        std::vector<uint8_t> bytes;
        export_bits(val, std::back_inserter(bytes), 8);

        // Pad with leading zeros to 32 bytes if necessary
        if (bytes.size() < 32) {
            std::vector<uint8_t> padded_bytes(32 - bytes.size(), 0);
            padded_bytes.insert(padded_bytes.end(), bytes.begin(), bytes.end());
            bytes = padded_bytes;
        }

        return toHex(bytes, withPrefix);
    }

    static std::string toHex(std::span<const std::uint8_t> bytes, bool withPrefix = false) {
        std::string hex;
        if (withPrefix) hex = "0x";
        boost::algorithm::hex(bytes.begin(), bytes.end(), std::back_inserter(hex));
        boost::algorithm::to_lower(hex);
        return hex;
    }

    static std::vector<uint8_t> fromHex(const std::string &hexStr) {
        std::string_view hexView(hexStr);
        if (hexView.substr(0, 2) == "0x" || hexView.substr(0, 2) == "0X") {
            hexView.remove_prefix(2);
        }
        if (hexView.size() % 2 != 0) {
            throw std::invalid_argument("Hex string must have even length");
        }
        std::vector<uint8_t> bytes;
        bytes.reserve(hexView.size() / 2);
        boost::algorithm::unhex(hexView.begin(), hexView.end(), std::back_inserter(bytes));
        return bytes;
    }

    static boost::multiprecision::uint256_t fromHexOrDecimal(const std::string &str) {
        if (str.empty())
            throw std::invalid_argument("Empty input for uint256");

        // ❌ Disallow any whitespace characters
        if (std::any_of(str.begin(), str.end(), ::isspace))
            throw std::invalid_argument("Whitespace not allowed in uint256 input");

        if (str[0] == '-')
            throw std::invalid_argument("Negative numbers not allowed for uint256");

        if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            auto bytes = fromHex(str);
            if (bytes.size() == 0)
                throw std::invalid_argument("Empty hex input for uint256");
            if (bytes.size() > 32)
                throw std::invalid_argument("Hex input too long for uint256");
            boost::multiprecision::uint256_t val = 0;
            for (auto b: bytes) {
                val = (val << 8) | b;
            }
            return val;
        } else {
            boost::multiprecision::uint256_t val(str);
            return val;
        }
    }

    // Convert uint256 to a base-10 string without leading zeros or prefix
    static std::string u256ToDecimal(const boost::multiprecision::uint256_t &val) {
        // Explicit base ensures decimal even under modified stream flags
        return val.str(0, std::ios_base::dec);
    }

    // Converts a hex string to a std::array<uint8_t, 32>, left-padded with zeros if necessary
    static std::array<uint8_t, 32> fromHexToArray32(const std::string &hexStr) {
        auto bytes = fromHex(hexStr);
        std::array<uint8_t, 32> arr{};
        if (bytes.size() > 32)
            throw std::invalid_argument("Hex input too long for array32");
        std::copy(bytes.begin(), bytes.end(), arr.begin() + (32 - bytes.size()));
        return arr;
    }

    // Converts a std::array<uint8_t, 32> to a hex string
    static std::string array32ToHex(const std::array<uint8_t, 32>& arr) {
        std::string hex;
        boost::algorithm::hex_lower(arr.begin(), arr.end(), std::back_inserter(hex));
        return hex;
    }

};
