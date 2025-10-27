#include "MachinePayCommon.h"
#include "EIP3009Authorization.h"
#include "crypto/Keccak.h"
#include "EthAddress.h"
#include "EIP712Signature.h"
#include "EIP712Domain.h"
#include "EIP3009Nonce.h"

using u256 = boost::multiprecision::uint256_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


static inline void packUint256(std::vector<uint8_t>& out, const u256& value) {
    constexpr size_t targetLen = 32;

    // Avoid extra reallocations if this is called in a tight loop.
    out.reserve(out.size() + targetLen);

    // export_bits(value, it, 8) writes big-endian without leading zeros.
    std::vector<uint8_t> tmp;
    tmp.reserve(targetLen);
    export_bits(value, std::back_inserter(tmp), 8); // big-endian, minimal length

    if (tmp.size() > targetLen) {
        // Should never happen for a true 256-bit type, but guard anyway.
        throw std::invalid_argument("packUint256: value does not fit in 32 bytes");
    }

    // Left-pad to 32 and append.
    out.insert(out.end(), targetLen - tmp.size(), 0);
    out.insert(out.end(), tmp.begin(), tmp.end());
}




// Helper to encode and hash the authorization message struct
static std::array<uint8_t, 32> hashTransferWithAuthorizationStruct(const EthAddress &from,
                                                                   const EthAddress &to,
                                                                   const u256 &value,
                                                                   const u256& validAfter,
                                                                   const u256& validBefore,
                                                                   const EIP3009Nonce &nonce) {

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
                                                        const EIP3009Nonce &nonce,
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
                                               const EIP3009Nonce &nonce,
                                               const EIP712Signature &signature) {
    auto structHash = hashTransferWithAuthorizationStruct(from, to, value, validAfter, validBefore, nonce);
    domain.verifyWithDomain(structHash, signature, from);
}
