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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Helper to pack a 64-bit unsigned integer into a byte array (big-endian)
static void packUint64(std::vector<uint8_t>& bytes, uint64_t value) {
    bytes.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Helper to encode and hash the authorization message
static std::array<uint8_t, 32> hashAuthorization(const EthAddress& from,
                                              const EthAddress& to,
                                              uint64_t value,
                                              uint64_t validAfter,
                                              uint64_t validBefore,
                                              const std::string& nonce) {
    std::vector<uint8_t> message;
    message.reserve(20 + 20 + 8 + 8 + 8 + 32); // Approximate size

    auto fromBytes = from.bytes();
    message.insert(message.end(), fromBytes.begin(), fromBytes.end());

    auto toBytes = to.bytes();
    message.insert(message.end(), toBytes.begin(), toBytes.end());

    packUint64(message, value);
    packUint64(message, validAfter);
    packUint64(message, validBefore);

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
                                       uint64_t value,
                                       uint64_t validAfter,
                                       uint64_t validBefore,
                                       const std::string& nonce,
                                       const EthPrivateKey& privateKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    return EthPrivateKey::signAuthRaw(hash.data(), privateKey.bytes().data());
}

void EIP3009Authorization::verifyAuthorization(const EthAddress& from,
                                  const EthAddress& to,
                                  uint64_t value,
                                  uint64_t validAfter,
                                  uint64_t validBefore,
                                  const std::string& nonce,
                                  const EIP712Signature& signature,
                                  const EthPublicKey& publicKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    EthPrivateKey::eip712VerifyRaw(hash.data(), signature.bytes().data(), publicKey.getAddress());
}
