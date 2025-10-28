#include "MachinePayCommon.h"
#include "PaymentRecord.h"
#include "crypto/Encoding.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EIP3009Nonce.h"
#include <soci/row.h>

ptr<PaymentRecord> PaymentRecord::deserializeFromDbRow(const soci::row &row) {
    auto fromAddress = EthAddress::parseHexAddress(row.get<std::string>("fromAddress"));
    EthAddress toAddress = EthAddress::parseHexAddress(row.get<std::string>("toAddress"));
    EIP3009Value value = EIP3009Value::fromHexOrDecimal(row.get<std::string>("value"));
    EIP3009Nonce nonce = EIP3009Nonce::fromHex(row.get<std::string>("nonce"));
    Hash resourceHash = Encoding::fromHexToHash(row.get<std::string>("hash"));
    Hash authorizationHash = Encoding::fromHexToHash(row.get<std::string>("authorizationHash"));
    Hash transactionHash = Encoding::fromHexToHash(row.get<std::string>("transactionHash"));
    auto timestamp = row.get<uint64_t>("timestamp");
    auto jsonInfo = row.get<std::string>("jsonInfo");
    return make_shared<PaymentRecord>(fromAddress, toAddress, value, nonce, resourceHash, timestamp,
                                      authorizationHash, transactionHash, jsonInfo);
}

EthAddress PaymentRecord::fromAddress() const {
    return fromAddress_;
}

EthAddress PaymentRecord::toAddress() const {
    return toAddress_;
}

EIP3009Value PaymentRecord::value() const {
    return value_;
}

EIP3009Nonce PaymentRecord::nonce() const {
    return nonce_;
}

Hash PaymentRecord::resourceHash() const {
    return resourceHash_;
}

uint64_t PaymentRecord::timestamp() const {
    return timestamp_;
}

Hash PaymentRecord::authorizationHash() const {
    return authorizationHash_;
}

Hash PaymentRecord::transactionHash() const {
    return transactionHash_;
}

std::string PaymentRecord::jsonInfo() const {
    return jsonInfo_;
}

PaymentRecord::PaymentRecord(const EthAddress &fromAddress,
                             const EthAddress &toAddress,
                             const EIP3009Value &value,
                             const EIP3009Nonce &nonce,
                             const Hash &resourceHash,
                             uint64_t timestamp,
                             const Hash &authorizationHash,
                             const Hash &transactionHash,
                             const std::string &jsonInfo)
    : fromAddress_(fromAddress),
      toAddress_(toAddress),
      value_(value),
      nonce_(nonce),
      resourceHash_(resourceHash),
      timestamp_(timestamp),
      authorizationHash_(authorizationHash),
      transactionHash_(transactionHash),
      jsonInfo_(jsonInfo) {
}
