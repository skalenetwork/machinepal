#pragma once
#include "crypto/EIP3009Nonce.h"
#include "crypto/EIP3009Value.h"
#include "crypto/Encoding.h"
#include "crypto/EthAddress.h"
#include <soci/row.h>

class EIP3009Nonce;
class EIP3009Value;
class EthAddress;
/**
 * @brief Represents a payment record.
 */
class PaymentRecord {
    std::string organizationName_;
    u256 chainId_;
    EthAddress fromAddress_;
    EthAddress toAddress_;
    EthAddress assetAddress_;
    EIP3009Value value_;
    EIP3009Nonce nonce_;
    Hash resourceHash_;
    uint64_t executionTime_;
    Hash authorizationSignatureHash_;
    Hash transactionHash_;
    std::string fromIpAddress_;
    std::string jsonInfo_;

public:

    [[nodiscard]] std::string organizationName() const;

    [[nodiscard]] u256 chainId() const;

    [[nodiscard]] EthAddress fromAddress() const;

    [[nodiscard]] EthAddress toAddress() const;

    [[nodiscard]] EthAddress assetAddress() const;

    [[nodiscard]] EIP3009Value value() const;

    [[nodiscard]] EIP3009Nonce nonce() const;

    [[nodiscard]] Hash resourceHash() const;

    [[nodiscard]] uint64_t executionTime() const;

    [[nodiscard]] Hash authorizationSignatureHash() const;

    [[nodiscard]] Hash transactionHash() const;

    std::string fromIpAddress() const;

    [[nodiscard]] std::string jsonInfo() const;


    PaymentRecord(const string& organizationName, const u256 chainId,
                    const EthAddress &fromAddress, const EthAddress &toAddress, const EthAddress &assetAddress,
                    const EIP3009Value &value,
                  const EIP3009Nonce &nonce, const Hash &resourceHash, uint64_t executionTime,
                  const Hash &authorizationSignatureHash, const Hash &transactionHash,
                  const std::string& fromIpAddress,
                  const std::string &jsonInfo);

    static ptr<PaymentRecord> deserializeFromDbRow(const soci::row &row);
};
