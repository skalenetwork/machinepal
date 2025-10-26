#pragma once
#include "boost/algorithm/hex.hpp"
#include "boost/algorithm/string/case_conv.hpp"

#include <iostream>
#include <span>

inline std::string toHex(std::span<const std::uint8_t> bytes, bool withPrefix = false) {
    std::string hex;
    if (withPrefix) hex = "0x";
    boost::algorithm::hex(bytes.begin(), bytes.end(), std::back_inserter(hex));
    boost::algorithm::to_lower(hex);
    return hex;
}

