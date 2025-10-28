#include "MachinePayCommon.h"
#include "PaymentRecord.h"
#include "crypto/Encoding.h"
#include "crypto/EthAddress.h"
#include "crypto/EIP3009Value.h"
#include "crypto/EIP3009Nonce.h"
#include <soci/row.h>

ptr<PaymentRecord> PaymentRecord::deserializeFromDbRow(const soci::row& row) {
    auto fromAddress = EthAddress::parseHexAddress(row.get<std::string>("fromAddress"));
    EthAddress toAddress = EthAddress::parseHexAddress(row.get<std::string>("toAddress"));
    EIP3009Value value = EIP3009Value::fromHexOrDecimal(row.get<std::string>("value"));
    EIP3009Nonce nonce = EIP3009Nonce::fromHex(row.get<std::string>("nonce"));
    Hash resourceHash = Encoding::fromHexToHash(row.get<std::string>("hash"));
    Hash transactionHash = Encoding::fromHexToHash(row.get<std::string>("transactionHash"));
    auto timestamp = row.get<uint64_t>("timestamp");
    auto jsonInfo = row.get<std::string>("jsonInfo");
    return make_shared<PaymentRecord>(fromAddress, toAddress, value, nonce, resourceHash, timestamp,
        transactionHash, jsonInfo);
}
