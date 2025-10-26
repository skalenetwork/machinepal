#include <boost/test/unit_test.hpp>
#include "crypto/EIP712Domain.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

BOOST_AUTO_TEST_CASE(EIP712Domain_Hash_Test) {
    auto hash = EIP712Domain::baseMainnet().hashDomain();
    std::string hashHex;
    boost::algorithm::hex(hash.begin(), hash.end(), std::back_inserter(hashHex));
    boost::algorithm::to_lower(hashHex);
    BOOST_TEST(hashHex == EIP712Domain::baseMainnet().domainSeparator());
}
