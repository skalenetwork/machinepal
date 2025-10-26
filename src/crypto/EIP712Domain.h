#pragma once
#include <string>
#include <boost/multiprecision/cpp_int.hpp>
#include "EthAddress.h"
#include <array>

using u256 = boost::multiprecision::uint256_t;

class EIP712Domain {
public:
    std::string name_;
    std::string version_;
    u256 chainId_;
    EthAddress verifyingContract_;

    // Default constructor
    EIP712Domain() = default;

    // Parameterized constructor
    EIP712Domain(const std::string &name, const std::string &version, const u256 &chainId,
                 const EthAddress &verifyingContract);

    // Computes the EIP-712 domain separator hash
    std::array<uint8_t, 32> hashDomain() const;
};

class USDCDomain : public EIP712Domain {
public:

    USDCDomain(const USDCDomain&) = delete;
    USDCDomain& operator=(const USDCDomain&) = delete;

    static const USDCDomain& BaseMainnet() {
        static USDCDomain instance_;
        return instance_;
    }

private:
    USDCDomain()
        : EIP712Domain("USD Coin", "2", 8453,
                       EthAddress("0x833589fCD6eDb6E08f4c7C32D4f71b54bdA02913")) {
    }
};