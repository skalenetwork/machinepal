#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Minimal Keccak-256 implementation interface.
// Implementation in Keccak.cpp (public domain style code).
namespace keccak {

std::array<uint8_t, 32> keccak256(std::span<const uint8_t> data);
std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data);
std::array<uint8_t, 32> keccak256(const std::string& ascii);

std::string keccak256Hex(std::span<const uint8_t> data);
std::string keccak256Hex(const std::string& ascii);

} // namespace keccak
