#include "EIP3009.h"
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

// Helper to encode and hash the authorization message
static std::array<uint8_t, 32> hashAuthorization(const EthAddress& from,
                                              const EthAddress& to,
                                              uint64_t value,
                                              uint64_t validAfter,
                                              uint64_t validBefore,
                                              const std::string& nonce) {
    // Concatenate and encode fields as per EIP-3009
    std::string message = from.toHex() + to.toHex() + std::to_string(value) +
                          std::to_string(validAfter) + std::to_string(validBefore) + nonce;
    return keccak::keccak256(message);
}

// Helper to convert bytes to hex string
static std::string bytesToHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return oss.str();
}

EIP712Signature EIP3009::signAuthorization(const EthAddress& from,
                                       const EthAddress& to,
                                       uint64_t value,
                                       uint64_t validAfter,
                                       uint64_t validBefore,
                                       const std::string& nonce,
                                       const EthPrivateKey& privateKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    return EthPrivateKey::signAuthRaw(hash.data(), privateKey.bytes().data());;
}

void EIP3009::verifyAuthorization(const EthAddress& from,
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
