#pragma once
#include <string>
#include "EthPrivateKey.h"
#include "EthPublicKey.h"
#include "EthAddress.h"
#include "EthSignature.h"

class EIP3009 {
public:
    // Signs an EIP-3009 authorization message
    static EthSignature signAuthorization(const EthAddress& from,
                                          const EthAddress& to,
                                          uint64_t value,
                                          uint64_t validAfter,
                                          uint64_t validBefore,
                                          const std::string& nonce,
                                          const EthPrivateKey& privateKey);

    // Verifies an EIP-3009 authorization signature
    static bool verifyAuthorization(const EthAddress& from,
                                    const EthAddress& to,
                                    uint64_t value,
                                    uint64_t validAfter,
                                    uint64_t validBefore,
                                    const std::string& nonce,
                                    const EthSignature& signature,
                                    const EthPublicKey& publicKey);
};
