#include "Keccak.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>

namespace {
// Rotation offsets
constexpr uint32_t R[25] = { 0,  36,   3,  41,  18,
                              1,  44,  10,  45,   2,
                             62,   6,  43,  15,  61,
                             28,  55,  25,  21,  56,
                             27,  20,  39,   8,  14 };

inline uint64_t rotl(uint64_t x, uint32_t n) { return (x << n) | (x >> (64 - n)); }

void keccakF1600(uint64_t s[25]) {
    static const uint64_t RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
        0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
        0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
        0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
        0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL };

    for (int round = 0; round < 24; ++round) {
        // Theta
        uint64_t C[5];
        for (int x = 0; x < 5; ++x) {
            C[x] = s[x] ^ s[x + 5] ^ s[x + 10] ^ s[x + 15] ^ s[x + 20];
        }
        uint64_t D[5];
        for (int x = 0; x < 5; ++x) {
            D[x] = C[(x + 4) % 5] ^ rotl(C[(x + 1) % 5], 1);
        }
        for (int i = 0; i < 25; i += 5) {
            for (int x = 0; x < 5; ++x) {
                s[i + x] ^= D[x];
            }
        }

        // Rho + Pi
        uint64_t B[25];
        for (int i = 0; i < 25; ++i) B[i] = 0;
        int x = 1, y = 0;
        uint64_t current = s[1];
        for (int i = 0; i < 24; ++i) {
            int idx = x + 5 * y;
            int rIndex = idx;
            int newX = y;
            int newY = (2 * x + 3 * y) % 5;
            uint64_t temp = s[idx];
            B[newX + 5 * newY] = rotl(current, R[idx]);
            current = temp;
            x = newX;
            y = newY;
        }
        B[0] = s[0];

        // Chi
        for (int i = 0; i < 25; i += 5) {
            uint64_t row[5];
            for (int j = 0; j < 5; ++j) row[j] = B[i + j];
            for (int j = 0; j < 5; ++j) {
                s[i + j] = row[j] ^ ((~row[(j + 1) % 5]) & row[(j + 2) % 5]);
            }
        }

        // Iota
        s[0] ^= RC[round];
    }
}
} // namespace

namespace keccak {

std::array<uint8_t, 32> keccak256(std::span<const uint8_t> data) {
    // Keccak-256: rate = 1088 bits = 136 bytes, capacity = 512 bits
    constexpr size_t rate = 136;
    uint64_t state[25];
    std::memset(state, 0, sizeof(state));

    size_t offset = 0;
    while (data.size() - offset >= rate) {
        for (size_t i = 0; i < rate; ++i) {
            reinterpret_cast<uint8_t*>(state)[i] ^= data[offset + i];
        }
        keccakF1600(state);
        offset += rate;
    }

    // Pad remainder
    uint8_t block[rate];
    std::memset(block, 0, rate);
    size_t remaining = data.size() - offset;
    if (remaining > 0) {
        std::memcpy(block, data.data() + offset, remaining);
    }
    // Keccak padding (multi-rate): append 0x01 then final bit 0x80 at last byte
    block[remaining] ^= 0x01; // may coincide with block[rate-1]
    block[rate - 1] ^= 0x80;
    for (size_t i = 0; i < rate; ++i) {
        reinterpret_cast<uint8_t*>(state)[i] ^= block[i];
    }
    keccakF1600(state);

    std::array<uint8_t, 32> out{};
    std::memcpy(out.data(), state, 32);
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

