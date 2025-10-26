#pragma once
#include "boost/algorithm/hex.hpp"
#include "boost/algorithm/string/case_conv.hpp"

#include <iostream>
#include <span>

class Hex {

public:

    static std::string toHex(std::span<const std::uint8_t> bytes, bool withPrefix = false) {
        std::string hex;
        if (withPrefix) hex = "0x";
        boost::algorithm::hex(bytes.begin(), bytes.end(), std::back_inserter(hex));
        boost::algorithm::to_lower(hex);
        return hex;
    }

    static std::vector<uint8_t> fromHex(const std::string& hexStr) {
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
};
