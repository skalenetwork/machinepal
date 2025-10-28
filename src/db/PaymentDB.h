//
// Created by kladko on 10/28/25.
//

#pragma once

#include <string>
#include <cstdint>

class MachinePayApp; // forward declaration

class PaymentDB {
public:
    explicit PaymentDB(MachinePayApp& app, const std::string &dataDir = "data");
    void writePayment(const std::string &from,
                      const std::string &to,
                      const std::string &value,
                      const std::string &nonce,
                      const std::string &resourceHash,
                      uint64_t timestamp,
                      const std::string &transactionHash,
                      const std::string &jsonInfo);
private:
    MachinePayApp& app_;
    std::string dbPath_;
    void ensureSchema();
};
