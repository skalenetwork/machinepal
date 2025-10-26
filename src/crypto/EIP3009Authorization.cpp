#include "EIP3009Authorization.h"
#include "crypto/Keccak.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <sstream>
#include <iomanip>
#include "EthAddress.h"
#include "EIP712Signature.h"
#include <boost/multiprecision/cpp_int.hpp>

using u256 = boost::multiprecision::uint256_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Helper to pack a 256-bit unsigned integer into a byte array (big-endian)
static void packUint256(std::vector<uint8_t>& bytes, const u256& value) {
    // export_bits exports as big-endian
    std::vector<unsigned char> temp_bytes;
    export_bits(value, std::back_inserter(temp_bytes), 8);

    // Pad with leading zeros to 32 bytes if necessary
    if (temp_bytes.size() < 32) {
        bytes.insert(bytes.end(), 32 - temp_bytes.size(), 0);
    }
    bytes.insert(bytes.end(), temp_bytes.begin(), temp_bytes.end());
}

// Helper to pack a 48-bit unsigned integer into a byte array (big-endian)
static void packUint48(std::vector<uint8_t>& bytes, uint64_t value) {
    bytes.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Helper to encode and hash the authorization message
// This corresponds to the EIP-3009 specification for the authorization structure.
static std::array<uint8_t, 32> hashAuthorization(const EthAddress& from,
                                              const EthAddress& to,
                                              const u256& value,
                                              uint64_t validAfter,
                                              uint64_t validBefore,
                                              const std::string& nonce) {
    const uint64_t MAX_UINT48 = 0xFFFFFFFFFFFF;
    if (validAfter > MAX_UINT48) {
        throw std::invalid_argument("validAfter exceeds uint48 max value");
    }
    if (validBefore > MAX_UINT48) {
        throw std::invalid_argument("validBefore exceeds uint48 max value");
    }

    std::vector<uint8_t> message;
    message.reserve(20 + 20 + 32 + 6 + 6 + 32); // Approximate size

    // EIP-3009 field: from (address)
    auto fromBytes = from.bytes();
    message.insert(message.end(), fromBytes.begin(), fromBytes.end());

    // EIP-3009 field: to (address)
    auto toBytes = to.bytes();
    message.insert(message.end(), toBytes.begin(), toBytes.end());

    // EIP-3009 field: value (uint256)
    packUint256(message, value);
    // EIP-3009 field: validAfter (uint48)
    packUint48(message, validAfter);
    // EIP-3009 field: validBefore (uint48)
    packUint48(message, validBefore);

    // EIP-3009 field: nonce (bytes32)
    // Assuming nonce is a 32-byte hex string without "0x" prefix
    std::vector<uint8_t> nonceBytes(32);
    for(size_t i = 0; i < 32; ++i) {
        nonceBytes[i] = std::stoul(nonce.substr(i * 2, 2), nullptr, 16);
    }
    message.insert(message.end(), nonceBytes.begin(), nonceBytes.end());

    return keccak::keccak256(message);
}

EIP712Signature EIP3009Authorization::signAuthorization(const EthAddress& from,
                                       const EthAddress& to,
                                       const u256& value,
                                       uint64_t validAfter,
                                       uint64_t validBefore,
                                       const std::string& nonce,
                                       const EthPrivateKey& privateKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    return EthPrivateKey::signAuthRaw(hash.data(), privateKey.bytes().data());
}

void EIP3009Authorization::verifyAuthorization(const EthAddress& from,
                                  const EthAddress& to,
                                  const u256& value,
                                  uint64_t validAfter,
                                  uint64_t validBefore,
                                  const std::string& nonce,
                                  const EIP712Signature& signature,
                                  const EthPublicKey& publicKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    EthPrivateKey::eip712VerifyRaw(hash.data(), signature.bytes().data(), publicKey.getAddress());
}
