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
    EthAddress fromAddress_;
    EthAddress toAddress_;
    EIP3009Value value_;
    EIP3009Nonce nonce_;
    Hash resourceHash_;
    uint64_t timestamp_;
    Hash authorizationHash_;
    Hash transactionHash_;
    std::string jsonInfo_;

public:

    [[nodiscard]] std::string organizationName() const;

    [[nodiscard]] EthAddress fromAddress() const;

    [[nodiscard]] EthAddress toAddress() const;

    [[nodiscard]] EIP3009Value value() const;

    [[nodiscard]] EIP3009Nonce nonce() const;

    [[nodiscard]] Hash resourceHash() const;

    [[nodiscard]] uint64_t timestamp() const;

    [[nodiscard]] Hash authorizationHash() const;

    [[nodiscard]] Hash transactionHash() const;

    [[nodiscard]] std::string jsonInfo() const;




    PaymentRecord(const string& organizationName, const EthAddress &fromAddress, const EthAddress &toAddress, const EIP3009Value &value,
                  const EIP3009Nonce &nonce, const Hash &resourceHash, uint64_t timestamp,
                  const Hash &authorizationHash, const Hash &transactionHash, const std::string &jsonInfo);

    static ptr<PaymentRecord> deserializeFromDbRow(const soci::row &row);
};
