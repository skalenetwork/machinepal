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
    EthAddress fromAddress_;
    EthAddress toAddress_;
    EIP3009Value value_;
    EIP3009Nonce nonce_;
    Hash resourceHash_;
    uint64_t timestamp_;
    Hash transactionHash_;
    std::string jsonInfo_;

public:
    [[nodiscard]] EthAddress fromAddress() const {
        return fromAddress_;
    }

    [[nodiscard]] EthAddress toAddress() const {
        return toAddress_;
    }

    [[nodiscard]] EIP3009Value value() const {
        return value_;
    }

    [[nodiscard]] EIP3009Nonce nonce() const {
        return nonce_;
    }

    [[nodiscard]] Hash resourceHash() const {
        return resourceHash_;
    }

    [[nodiscard]] uint64_t timestamp() const {
        return timestamp_;
    }

    [[nodiscard]] Hash transactionHash() const {
        return transactionHash_;
    }

    [[nodiscard]] std::string jsonInfo() const {
        return jsonInfo_;
    }


    PaymentRecord(const EthAddress &fromAddress,
                  const EthAddress &toAddress,
                  const EIP3009Value &value,
                  const EIP3009Nonce &nonce,
                  const Hash &resourceHash,
                  uint64_t timestamp,
                  const Hash &transactionHash,
                  const std::string &jsonInfo)
        : fromAddress_(fromAddress.toChecksumHex()),
          toAddress_(toAddress.toChecksumHex()),
          value_(value),
          nonce_(nonce),
          resourceHash_(resourceHash),
          timestamp_(timestamp),
          transactionHash_(transactionHash),
          jsonInfo_(jsonInfo) {
    }

    static ptr<PaymentRecord> deserializeFromDbRow(const soci::row &row);
};
