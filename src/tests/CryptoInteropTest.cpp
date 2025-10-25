#include <boost/test/unit_test.hpp>
#include "crypto/EthPrivateKey.h"
#include "crypto/EthAddress.h"
#include "crypto/CryptoManager.h"
#include "crypto/Keccak.h"
#include <sstream>
#include <iomanip>


// Helper: convert std::array<uint8_t, 32> to lowercase hex string
static std::string to_hex(const std::array<uint8_t, 32>& arr) {
    std::ostringstream oss;
    for (auto b : arr) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return oss.str();
}


BOOST_AUTO_TEST_CASE(keccak256_abc_vector)
{
    const std::vector<uint8_t> input = {'a', 'b', 'c'};

    // Known correct Ethereum keccak256("abc") output
    const std::string expected =
        "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45";

    const auto digest = keccak::keccak256(input);
    const std::string got = to_hex(digest);

    BOOST_CHECK_MESSAGE(
        got == expected,
        "Keccak256('abc') mismatch:\n  expected = " << expected <<
        "\n  got      = " << got);
}

// Deterministic test using private key = 1
BOOST_AUTO_TEST_CASE(private_key_scalar_one) {
    std::string privateKeyStr = "0x" + std::string(63, '0') + "1"; // 64 hex chars ending with 1
    auto privateKey = EthPrivateKey::parseFlexible(privateKeyStr);


    auto publicKey = CryptoManager::derivePublicKeyFromPrivateKey(privateKey);

    // Expected uncompressed public key (65 bytes, 0x04 prefix)
    const std::string expectedPublicKeyHex =
        "0x0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8";

    BOOST_TEST_MESSAGE("Public key (uncompressed): " << publicKey.toHex());
    BOOST_TEST(publicKey.toHex() == expectedPublicKeyHex);


    auto addr = publicKey.getAddress();

    BOOST_TEST(addr.toHex() == "0x8626f6940e2eb28930efb4cef49b2d1f2c9c1199");
    BOOST_TEST(addr.toChecksumHex() == "0x8626f6940E2eb28930eFb4CeF49B2d1F2C9C1199");
}

BOOST_AUTO_TEST_CASE(private_key_no_prefix_scalar_one) {
    std::string pkHexNoPrefix = std::string(63, '0') + "1";
    auto pk = EthPrivateKey::parseFlexible(pkHexNoPrefix);
    BOOST_TEST(pk.toHex() == "0x" + pkHexNoPrefix);
}

BOOST_AUTO_TEST_CASE(invalid_hex_rejected) {
    std::string bad = "0x" + std::string(62, '0') + "Z1"; // invalid char Z
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(bad), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(checksum_validation_scalar_one_address) {
    std::string checksumAddr = "0x8626f6940E2eb28930eFb4CeF49B2d1F2C9C1199";
    auto addr = EthAddress::parseFlexible(checksumAddr, true);
    BOOST_TEST(addr.toHex() == "0x8626f6940e2eb28930efb4cef49b2d1f2c9c1199");
}

BOOST_AUTO_TEST_CASE(range_checks) {
    std::string zeroKey = "0x" + std::string(64, '0');
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(zeroKey), std::invalid_argument);
    std::string n = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"; // n invalid
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(n), std::invalid_argument);
    std::string nMinus1 = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140"; // valid no prefix
    auto pk = EthPrivateKey::parseFlexible(nMinus1);
    std::string expectedLower;
    expectedLower.reserve(2 + nMinus1.size());
    expectedLower += "0x";
    for(char c: nMinus1) expectedLower.push_back(std::tolower(static_cast<unsigned char>(c)));
    BOOST_TEST(pk.toHex() == expectedLower);
}

BOOST_AUTO_TEST_CASE(generate_pair_round_trip) {
    auto [pk, addr] = CryptoManager::generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair();
    auto derived = CryptoManager::derivePublicKeyFromPrivateKey(pk);
    BOOST_TEST(derived.toHex() == addr.toHex());
}

BOOST_AUTO_TEST_CASE(known_vector_0x4c0883) {
    std::string pkHex = "0x4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f";
    auto pk = EthPrivateKey::parseFlexible(pkHex);
    auto addr = CryptoManager::derivePublicKeyFromPrivateKey(pk).getAddress();
    BOOST_TEST(addr.toHex() == "0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1");
    BOOST_TEST(addr.toChecksumHex() == "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1");
}

// TODO: Verify private key scalar one test after keccak fix
