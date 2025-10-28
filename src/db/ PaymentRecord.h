#pragma once
#include "crypto/EIP3009Nonce.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EthAddress.h"

class EIP3009Nonce;
class EIP3009Value;
class EthAddress;
/**
 * @brief Represents a payment record.
 */
class PaymentRecord {
public:
    std::string fromAddress;
    std::string toAddress;
    std::string value;
    std::string nonce;
    std::string resourceHash;
    uint64_t timestamp;
    std::string transactionHash;
    std::string jsonInfo;

    PaymentRecord(const std::string& fromAddress,
                  const std::string& toAddress,
                  const std::string& value,
                  const std::string& nonce,
                  const std::string& resourceHash,
                  uint64_t timestamp,
                  const std::string& transactionHash,
                  const std::string& jsonInfo)
        : fromAddress(fromAddress),
          toAddress(toAddress),
          value(value),
          nonce(nonce),
          resourceHash(resourceHash),
          timestamp(timestamp),
          transactionHash(transactionHash),
          jsonInfo(jsonInfo) {}

    PaymentRecord(const EthAddress& fromAddress,
                  const EthAddress& toAddress,
                  const EIP3009Value& value,
                  const EIP3009Nonce& nonce,
                  const std::string& resourceHash,
                  uint64_t timestamp,
                  const std::string& transactionHash,
                  const std::string& jsonInfo)
        : fromAddress(fromAddress.toChecksumHex()),
          toAddress(toAddress.toChecksumHex()),
          value(value.toDecimal()),
          nonce(nonce.toHex()),
          resourceHash(resourceHash),
          timestamp(timestamp),
          transactionHash(transactionHash),
          jsonInfo(jsonInfo) {}
};