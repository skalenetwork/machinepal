#pragma once
#include <string>
#include <boost/multiprecision/cpp_int.hpp>
#include "EthPrivateKey.h"
#include "EthPublicKey.h"
#include "EthAddress.h"
#include "EIP712Signature.h"
#include "EIP712Domain.h"

using u256 = boost::multiprecision::uint256_t;

class EIP3009Authorization {
public:
    // Signs an EIP-3009 authorization message
    static EIP712Signature signAuthorization(const EIP712Domain& domain,
                                          const EthAddress& from,
                                          const EthAddress& to,
                                          const u256& value,
                                          uint64_t validAfter,
                                          uint64_t validBefore,
                                          const std::string& nonce,
                                          const EthPrivateKey& privateKey);

    // Verifies an EIP-3009 authorization signature
    static void verifyAuthorization(const EIP712Domain& domain,
                                    const EthAddress& from,
                                    const EthAddress& to,
                                    const u256& value,
                                    uint64_t validAfter,
                                    uint64_t validBefore,
                                    const std::string& nonce,
                                    const EIP712Signature& signature,
                                    const EthPublicKey& publicKey);
};
