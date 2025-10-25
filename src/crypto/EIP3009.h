#pragma once
#include <string>
#include "EthPrivateKey.h"
#include "EthPublicKey.h"
#include "EthAddress.h"
#include "EIP712Signature.h"

class EIP3009 {
public:
    // Signs an EIP-3009 authorization message
    static EIP712Signature signAuthorization(const EthAddress& from,
                                          const EthAddress& to,
                                          uint64_t value,
                                          uint64_t validAfter,
                                          uint64_t validBefore,
                                          const std::string& nonce,
                                          const EthPrivateKey& privateKey);

    // Verifies an EIP-3009 authorization signature
    static void verifyAuthorization(const EthAddress& from,
                                    const EthAddress& to,
                                    uint64_t value,
                                    uint64_t validAfter,
                                    uint64_t validBefore,
                                    const std::string& nonce,
                                    const EIP712Signature& signature,
                                    const EthPublicKey& publicKey);
};
