#include <boost/test/unit_test.hpp>
#include "crypto/EIP712Domain.h"
#include "crypto/Hex.h"

BOOST_AUTO_TEST_CASE(EIP712Domain_TypeHash_Test) {
    std::string expected_hash_hex = "8b73c3c69bb8fe3d512ecc4cf759cc79239f7b179b0ffacaa9a75d522b39400f";
    auto expected_hash_bytes = hex::decode(expected_hash_hex);
    std::array<uint8_t, 32> expected_hash_array;
    std::copy(expected_hash_bytes.begin(), expected_hash_bytes.end(), expected_hash_array.begin());

    BOOST_CHECK_EQUAL_COLLECTIONS(EIP712_DOMAIN_TYPEHASH.begin(), EIP712_DOMAIN_TYPEHASH.end(),
                                  expected_hash_array.begin(), expected_hash_array.end());

    BOOST_CHECK(EIP712Domain::baseMainnet().hashDomain() = EIP712Domain::baseMainnet().domain_separator());
}




