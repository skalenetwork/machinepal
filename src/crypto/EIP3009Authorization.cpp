#include "MachinePayCommon.h"
#include "EIP3009Authorization.h"
#include "crypto/Keccak.h"
#include "EthAddress.h"
#include "EIP712Signature.h"
#include "EIP712Domain.h"

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

// EIP-3009 Authorization Type Hash
const std::array<uint8_t, 32> EIP3009_AUTHORIZATION_TYPEHASH = keccak::keccak256("Authorization(address from,address to,uint256 value,uint48 validAfter,uint48 validBefore,bytes32 nonce)");

// Helper to encode and hash the authorization message struct
static std::array<uint8_t, 32> hashAuthorizationStruct(const EthAddress& from,
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
    message.reserve(32 + 20 + 20 + 32 + 6 + 6 + 32); // typehash + from + to + value + validAfter + validBefore + nonce

    message.insert(message.end(), EIP3009_AUTHORIZATION_TYPEHASH.begin(), EIP3009_AUTHORIZATION_TYPEHASH.end());

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

static std::array<uint8_t, 32> getEIP712Hash(const EIP712Domain& domain, const std::array<uint8_t, 32>& structHash) {
    std::vector<uint8_t> data_to_hash;
    data_to_hash.push_back(0x19);
    data_to_hash.push_back(0x01);

    auto domain_separator = domain.hashDomain();
    data_to_hash.insert(data_to_hash.end(), domain_separator.begin(), domain_separator.end());
    data_to_hash.insert(data_to_hash.end(), structHash.begin(), structHash.end());

    return keccak::keccak256(data_to_hash);
}

EIP712Signature EIP3009Authorization::signAuthorization(const EIP712Domain& domain,
                                       const EthAddress& from,
                                       const EthAddress& to,
                                       const u256& value,
                                       uint64_t validAfter,
                                       uint64_t validBefore,
                                       const std::string& nonce,
                                       const EthPrivateKey& privateKey) {
    auto structHash = hashAuthorizationStruct(from, to, value, validAfter, validBefore, nonce);
    auto finalHash = getEIP712Hash(domain, structHash);
    return EthPrivateKey::signAuthRaw(finalHash.data(), privateKey.bytes().data());
}

void EIP3009Authorization::verifyAuthorization(const EIP712Domain& domain,
                                  const EthAddress& from,
                                  const EthAddress& to,
                                  const u256& value,
                                  uint64_t validAfter,
                                  uint64_t validBefore,
                                  const std::string& nonce,
                                  const EIP712Signature& signature,
                                  const EthPublicKey& publicKey) {
    auto structHash = hashAuthorizationStruct(from, to, value, validAfter, validBefore, nonce);
    auto finalHash = getEIP712Hash(domain, structHash);
    EthPrivateKey::eip712VerifyRaw(finalHash.data(), signature.bytes().data(), publicKey.getAddress());
}
