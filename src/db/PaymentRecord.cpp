#include "MachinePayCommon.h"
#include "PaymentRecord.h"
#include "crypto/Encoding.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EIP3009Nonce.h"
#include <soci/row.h>

ptr<PaymentRecord> PaymentRecord::deserializeFromDbRow(const soci::row &) {
    /*
    auto organizationName = row.get<std::string>("organizationName");
        u256 chainId = Encoding::u256FromHexOrDecimal(row.get<std::string>("chainId"));
    EthAddress toAddress = EthAddress::parseHexAddress(row.get<std::string>("toAddress"));
    EthAddress assetAddress = EthAddress::parseHexAddress(row.get<std::string>("assetAddress"));
    EIP3009Value value = EIP3009Value::fromHexOrDecimal(row.get<std::string>("value"));
    EIP3009Nonce nonce = EIP3009Nonce::fromHex(row.get<std::string>("nonce"));
    Hash resourceHash = Encoding::fromHexToHash(row.get<std::string>("resourceHash"));
    Hash authorizationHash = Encoding::fromHexToHash(row.get<std::string>("authorizationHash"));
    Hash transactionHash = Encoding::fromHexToHash(row.get<std::string>("transactionHash"));
    auto executionTime = static_cast<uint64_t>(row.get<long long>("executionTime"));
    auto fromIpAddress = row.get<std::string>("fromIpAddress");
    auto jsonInfo = row.get<std::string>("jsonInfo");
    return make_shared<PaymentRecord>(organizationName, chainId, fromAddress, toAddress, assetAddress, value, nonce, resourceHash, executionTime,
                                      authorizationHash, transactionHash, fromIpAddress, jsonInfo);
 (*/
    throw std::runtime_error("PaymentRecord::deserializeFromDbRow not implemented");
    return nullptr;
}

std::string PaymentRecord::organizationName() const {
    return organizationName_;
}

u256 PaymentRecord::chainId() const {
    return chainId_;
}

EthAddress PaymentRecord::fromAddress() const {
    return fromAddress_;
}

EthAddress PaymentRecord::toAddress() const {
    return toAddress_;
}

EthAddress PaymentRecord::assetAddress() const {
    return assetAddress_;
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

uint64_t PaymentRecord::executionTime() const {
    return executionTime_;
}

Hash PaymentRecord::authorizationSignatureHash() const {
    return authorizationSignatureHash_;
}

Hash PaymentRecord::transactionHash() const {
    return transactionHash_;
}

std::string PaymentRecord::fromIpAddress() const {
    return fromIpAddress_;
}

std::string PaymentRecord::jsonInfo() const {
    return jsonInfo_;
}

PaymentRecord::PaymentRecord(const string &organizationName, const u256 chainId, const EthAddress &fromAddress,
                             const EthAddress &toAddress, const EthAddress &assetAddress,
                             const EIP3009Value &value,
                             const EIP3009Nonce &nonce,
                             const Hash &resourceHash,
                             uint64_t executionTime,
                             const Hash &authorizationSignatureHash,
                             const Hash &transactionHash,
                             const std::string &fromIpAddress,
                             const std::string &jsonInfo)
    : organizationName_(organizationName),
      chainId_(chainId),
      fromAddress_(fromAddress),
      toAddress_(toAddress),
        assetAddress_(assetAddress),
      value_(value),
      nonce_(nonce),
      resourceHash_(resourceHash),
      executionTime_(executionTime),
      authorizationSignatureHash_(authorizationSignatureHash),
      transactionHash_(transactionHash),
      fromIpAddress_(fromIpAddress),
      jsonInfo_(jsonInfo) {
}