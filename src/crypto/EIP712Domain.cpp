#include "MachinePayCommon.h"

#include "EIP712Domain.h"
#include "Keccak.h"
#include <vector>


// https://github.com/0xsequence/ethers-eip712/blob/master/tests/typed-data.test.ts
std::array<uint8_t, 32> EIP712Domain::getDomainTypeHash() {
    // EIP-712 Domain Type Hash
    //  public constant EIP712_DOMAIN_TYPEHASH = 0x8b73c3c69bb8fe3d512ecc4cf759cc79239f7b179b0ffacaa9a75d522b39400f;
    static std::array<uint8_t, 32> EIP712_DOMAIN_TYPEHASH = keccak::keccak256(
        "EIP712Domain(string name,string version,uint256 chainId,address verifyingContract)");

    std::string hex;
    boost::algorithm::hex(EIP712_DOMAIN_TYPEHASH.begin(), EIP712_DOMAIN_TYPEHASH.end(), std::back_inserter(hex));
    boost::algorithm::to_lower(hex);

    CHECK_STATE(hex== "8b73c3c69bb8fe3d512ecc4cf759cc79239f7b179b0ffacaa9a75d522b39400f");

    return EIP712_DOMAIN_TYPEHASH;
}



// https://github.com/0xsequence/ethers-eip712/blob/master/tests/typed-data.test.ts
std::array<uint8_t, 32> EIP712Domain::hashDomain() const {
    std::vector<uint8_t> encodedData;

    // EIP-712 field: typeHash
    encodedData.insert(encodedData.end(), getDomainTypeHash().begin(), getDomainTypeHash().end());

    // EIP-712 field: name (string)
    auto hashed_name = keccak::keccak256(name_);
    encodedData.insert(encodedData.end(), hashed_name.begin(), hashed_name.end());

    // EIP-712 field: version (string)
    auto hashed_version = keccak::keccak256(version_);
    encodedData.insert(encodedData.end(), hashed_version.begin(), hashed_version.end());

    // EIP-712 field: chainId (uint256)
    std::vector<uint8_t> chainIdBytes;
    boost::multiprecision::export_bits(chainId_, std::back_inserter(chainIdBytes), 8);
    if (chainIdBytes.size() < 32) {
        std::vector<uint8_t> padding(32 - chainIdBytes.size(), 0);
        encodedData.insert(encodedData.end(), padding.begin(), padding.end());
    }
    encodedData.insert(encodedData.end(), chainIdBytes.begin(), chainIdBytes.end());

    // EIP-712 field: verifyingContract (address)
    auto contractBytes = verifyingContract_.bytes();
    std::vector<uint8_t> paddedContract(12, 0);
    paddedContract.insert(paddedContract.end(), contractBytes.begin(), contractBytes.end());
    encodedData.insert(encodedData.end(), paddedContract.begin(), paddedContract.end());

    return keccak::keccak256(encodedData);
}
