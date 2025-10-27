#include "MachinePayCommon.h"
#include "EIP3009Authorization.h"
#include "crypto/Keccak.h"
#include "EthAddress.h"
#include "EIP712Signature.h"
#include "EIP712Domain.h"
#include "EIP3008Nonce.h"

using u256 = boost::multiprecision::uint256_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Helper to pack a 256-bit unsigned integer into a byte array (big-endian)
static void packUint256(std::vector<uint8_t> &bytes, const u256 &value) {
    // export_bits exports as big-endian
    std::vector<unsigned char> temp_bytes;
    export_bits(value, std::back_inserter(temp_bytes), 8);

    // Pad with leading zeros to 32 bytes if necessary
    if (temp_bytes.size() < 32) {
        bytes.insert(bytes.end(), 32 - temp_bytes.size(), 0);
    }
    bytes.insert(bytes.end(), temp_bytes.begin(), temp_bytes.end());
}


// Helper to encode and hash the authorization message struct
static std::array<uint8_t, 32> hashTransferWithAuthorizationStruct(const EthAddress &from,
                                                                   const EthAddress &to,
                                                                   const u256 &value,
                                                                   const u256 validAfter,
                                                                   const u256 validBefore,
                                                                   const EIP3008Nonce &nonce) {

    std::vector<uint8_t> message;
    message.reserve(7 * 32); // 7 fields * 32 bytes each

    static auto transferWithAuthorizationTypeHashVector = Hex::fromHex(
        EIP3009Authorization::TRANSFER_WITH_AUTHORIZATION_TYPE_HASH);
    message.insert(message.end(), transferWithAuthorizationTypeHashVector.begin(),
                   transferWithAuthorizationTypeHashVector.end());

    // EIP-3009 field: from (address) - padded to 32 bytes
    auto fromBytes = from.bytes();
    message.insert(message.end(), 32 - fromBytes.size(), 0);
    message.insert(message.end(), fromBytes.begin(), fromBytes.end());

    // EIP-3009 field: to (address) - padded to 32 bytes
    auto toBytes = to.bytes();
    message.insert(message.end(), 32 - toBytes.size(), 0);
    message.insert(message.end(), toBytes.begin(), toBytes.end());

    // EIP-3009 field: value (uint256)
    packUint256(message, value);

    packUint256(message, validAfter);

    packUint256(message, validBefore);

    // EIP-3009 field: nonce (bytes32)
    const auto& nonceBytes = nonce.bytes();
    message.insert(message.end(), nonceBytes.begin(), nonceBytes.end());

    return keccak::keccak256(message);
}


EIP712Signature EIP3009Authorization::signAuthorization(const EIP712Domain &domain,
                                                        const EthAddress &from,
                                                        const EthAddress &to,
                                                        const u256& value,
                                                        const u256& validAfter,
                                                        const u256& validBefore,
                                                        const EIP3008Nonce &nonce,
                                                        const EthPrivateKey &privateKey) {
    auto structHash = hashTransferWithAuthorizationStruct(from, to, value, validAfter, validBefore, nonce);
    return domain.signWithDomain(structHash, privateKey);

}

void EIP3009Authorization::verifyAuthorization(const EIP712Domain &domain,
                                               const EthAddress &from,
                                               const EthAddress &to,
                                               const u256 &value,
                                               const u256& validAfter,
                                               const u256& validBefore,
                                               const EIP3008Nonce &nonce,
                                               const EIP712Signature &signature,
                                               const EthPublicKey &publicKey) {
    auto structHash = hashTransferWithAuthorizationStruct(from, to, value, validAfter, validBefore, nonce);
    domain.verifyWithDomain(structHash, signature, publicKey);
}
