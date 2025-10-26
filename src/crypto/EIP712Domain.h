#pragma once
#include <string>
#include <boost/multiprecision/cpp_int.hpp>
#include "EthAddress.h"
#include "Hex.h"
#include <optional>
#include <array>


// https://www.circle.com/multi-chain-usdc

using u256 = boost::multiprecision::uint256_t;

class EIP712Domain {
    std::string name_;
    std::string version_;
    u256 chainId_;
    EthAddress verifyingContract_;
    std::string domainSeparator_;


public:


    [[nodiscard]] std::string domainSeparator() const {
        return domainSeparator_;
    }


    // Default constructor
    EIP712Domain() = default;

    EIP712Domain(const std::string &name, const std::string &version, const u256 &chainId,
                 const EthAddress &verifyingContract, const std::optional<std::string> domainSeparator
                 = std::nullopt);

    // Computes the EIP-712 domain separator hash
    std::array<uint8_t, 32> hashDomain() const;

    static std::array<uint8_t, 32> getDomainTypeHash();


    static EIP712Domain machinePayEasyTestNet() {
        return EIP712Domain{"USDC", "2", 84542,
                            EthAddress("0x036CbD53842c5426634e7929541eC2318f3dCF7e")
        };
    }

    static EIP712Domain baseSepolia() {
        return EIP712Domain{"USDC", "2", 84532,
                            EthAddress("0x036CbD53842c5426634e7929541eC2318f3dCF7e"),
                            "0x71f17a3b2ff373b803d70a5a07c046c1a2bc8e89c09ef722fcb047abe94c9818"
        };
    }


    static EIP712Domain baseMainnet() {
        return {"USD Coin", "2", 8453,
                EthAddress("0x833589fCD6eDb6E08f4c7C32D4f71b54bdA02913"),
                "0x02fa7265e7c5d81118673727957699e4d68f74cd74b7db77da710fe8a2c7834f"};
    }

public:
    [[nodiscard]] std::string name() const {
        return name_;
    }

    [[nodiscard]] std::string version() const {
        return version_;
    }

    [[nodiscard]] u256 chainId() const {
        return chainId_;
    }

    [[nodiscard]] EthAddress verifyingContract() const {
        return verifyingContract_;
    }

    std::array<uint8_t, 32> hashDomainWithStruct(const std::array<uint8_t, 32>& structHash) const;

};