#include <crypto/Hex.h>
#include <boost/test/unit_test.hpp>
#include "crypto/EIP712Domain.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

BOOST_AUTO_TEST_CASE(EIP712Domain_Hash_Test) {
    auto hash = EIP712Domain::baseMainnet().hashDomain();
    BOOST_TEST(Hex::toHex(hash, true) == EIP712Domain::baseMainnet().domainSeparator());
    auto hash2 = EIP712Domain::baseSepolia().hashDomain();
    BOOST_TEST(Hex::toHex(hash2, true) == EIP712Domain::baseSepolia().domainSeparator());

}
