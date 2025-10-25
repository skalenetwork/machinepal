#include <boost/test/unit_test.hpp>
#include "crypto/EIP3009.h"
#include "crypto/EthPrivateKey.h"
#include "crypto/EthPublicKey.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP712Signature.h"

BOOST_AUTO_TEST_CASE(EIP3009_SignAndVerify_ReferenceValues) {
    // Reference values (example test vectors)
    EthAddress from("0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1");
    EthAddress to("0xffcf8fdee72ac11b5c542428b35eef5769c409f0");
    uint64_t value = 1000000000000000000ULL; // 1 ETH in wei
    uint64_t validAfter = 1633046400; // 2021-10-01
    uint64_t validBefore = 1733046400; // 2024-10-01
    std::string nonce = "0xabcdef1234567890";

    // Example private key (DO NOT USE IN PRODUCTION)
    std::string privKeyHex = "4c0883a69102937d6231471b5dbb6204fe5129617082796e8a7a7e7a7a7a7a7a";
    EthPrivateKey privKey(privKeyHex);
    EthPublicKey pubKey = privKey.computePublicKey();

    // Sign authorization
    EIP712Signature signature = EIP3009::signAuthorization(from, to, value, validAfter, validBefore, nonce, privKey);
    BOOST_TEST(!signature.toHex().empty());

    // Verify authorization
    EIP3009::verifyAuthorization(from, to, value, validAfter, validBefore, nonce, signature, pubKey);


}
