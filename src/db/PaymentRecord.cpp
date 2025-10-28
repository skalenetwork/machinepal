#include "MachinePayCommon.h"
#include "PaymentRecord.h"

PaymentRecord::PaymentRecord(const std::string& fromAddress,
                             const std::string& toAddress,
                             const std::string& value,
                             const std::string& nonce,
                             const std::string& resourceHash,
                             uint64_t timestamp,
                             const std::string& transactionHash,
                             const std::string& jsonInfo)
    : fromAddress_(EthAddress::parseFlexible(fromAddress)),
      toAddress_(EthAddress::parseFlexible(toAddress)),
      value_(EIP3009Value::fromDecimal(value)),
      nonce_(EIP3009Nonce::fromHex(nonce)),
      resourceHash_(Encoding::fromHexToArray32(resourceHash)),
      timestamp_(timestamp),
      transactionHash_(transactionHash),
      jsonInfo_(jsonInfo) {}

PaymentRecord::PaymentRecord(const EthAddress& fromAddress,
                             const EthAddress& toAddress,
                             const EIP3009Value& value,
                             const EIP3009Nonce& nonce,
                             const Hash& resourceHash,
                             uint64_t timestamp,
                             const std::string& transactionHash,
                             const std::string& jsonInfo)
    : fromAddress_(fromAddress),
      toAddress_(toAddress),
      value_(value),
      nonce_(nonce),
      resourceHash_(resourceHash),
      timestamp_(timestamp),
      transactionHash_(transactionHash),
      jsonInfo_(jsonInfo) {}

// Accessors are defined inline in the header.

