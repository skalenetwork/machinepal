#include <boost/test/unit_test.hpp>
#include "crypto/EthPrivateKey.h"
#include "crypto/EthAddress.h"
#include "crypto/CryptoManager.h"
#include "crypto/Keccak.h"

BOOST_AUTO_TEST_CASE(known_private_key_derives_expected_address) {
    // Test vector: 0x4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f
    // Expected address: 0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1 (checksummed 0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1)
    std::string privHex = "0x4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f";
    auto pk = EthPrivateKey::parseFlexible(privHex);
    auto addr = CryptoManager::deriveAddressFromPrivateKey(pk);
    BOOST_TEST(addr.toHex() == "0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1");
    BOOST_TEST(addr.toChecksumHex() == "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1");
}

BOOST_AUTO_TEST_CASE(known_private_key_vector_4c0883) {
    std::string privHex = "0x4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f";
    auto pk = EthPrivateKey::parseFlexible(privHex);
    auto addr = CryptoManager::deriveAddressFromPrivateKey(pk);
    BOOST_TEST(addr.toHex() == "0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1");
    BOOST_TEST(addr.toChecksumHex() == "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1");
}

BOOST_AUTO_TEST_CASE(private_key_no_prefix_parses) {
    std::string privHexNoPrefix = "4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f";
    auto pk = EthPrivateKey::parseFlexible(privHexNoPrefix);
    BOOST_TEST(pk.toHex() == "0x4c0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f");
}

BOOST_AUTO_TEST_CASE(invalid_hex_private_key_rejected) {
    std::string invalidHex = "0xZZ0883a69102937d6231471b5dbb6204fe5129617082790839b22c7f81b0e6f";
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(invalidHex), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(checksum_parsing_validation) {
    std::string checksum = "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1";
    auto addr = EthAddress::parseFlexible(checksum, true);
    BOOST_TEST(addr.toHex() == "0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1");
    // Alter one casing to force mismatch
    std::string bad = "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9c2"; // last char changed
    BOOST_CHECK_THROW(EthAddress::parseFlexible(bad, true), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(address_parse_without_checksum_validation_accepts_lowercase) {
    std::string lowercase = "0x90f8bf6a479f320ead074411a4b0e7944ea8c9c1";
    auto addr = EthAddress::parseFlexible(lowercase, false);
    BOOST_TEST(addr.toChecksumHex() == "0x90F8bf6A479f320eAd074411a4B0e7944Ea8c9C1");
}

BOOST_AUTO_TEST_CASE(private_key_range_checks) {
    // Zero key should fail
    std::string zeroKey(66, '0'); zeroKey[0]='0'; zeroKey[1]='x';
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(zeroKey), std::invalid_argument);
    // Upper bound (n) should fail: use n directly
    std::string n = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"; // equals n (invalid)
    BOOST_CHECK_THROW(EthPrivateKey::parseFlexible(n), std::invalid_argument);
    // Valid case: n-1
    std::string nMinus1 = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140"; // n-1 valid
    auto pk = EthPrivateKey::parseFlexible(nMinus1);
    BOOST_TEST(pk.toHex() == nMinus1);
}

BOOST_AUTO_TEST_CASE(generate_pair_round_trip) {
    auto [pk, addr] = CryptoManager::generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair();
    auto derived = CryptoManager::deriveAddressFromPrivateKey(pk);
    BOOST_TEST(derived.toHex() == addr.toHex());
    BOOST_TEST(derived.toChecksumHex() == addr.toChecksumHex());
}
