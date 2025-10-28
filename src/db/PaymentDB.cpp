#include "MachinePayCommon.h"

#include "PaymentDB.h"

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <filesystem>

using namespace std;

PaymentDB::PaymentDB(MachinePayApp& app, const std::string &dataDir)
    : app_(app)
{
    try {
        std::filesystem::create_directories(dataDir); // idempotent
        dbPath_ = dataDir + "/machinepay.db";
        ensureSchema();
    } catch(...) {
        RETHROW_NESTED2("Failed to initialize PaymentDB");
    }
}

void PaymentDB::ensureSchema() {
    try {
        soci::session sql(soci::sqlite3, dbPath_);
        sql << "CREATE TABLE IF NOT EXISTS payments ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "from TEXT NOT NULL,"
               "to TEXT NOT NULL,"
               "value TEXT NOT NULL,"
               "nonce TEXT NOT NULL,"
               "hash TEXT NOT NULL,"
               "timestamp INTEGER NOT NULL,"
               "transactionHash TEXT NOT NULL,"
               "jsonInfo TEXT)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_hash ON payments(hash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_txhash ON payments(transactionHash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_nonce ON payments(nonce)";
    } catch(...) {
        RETHROW_NESTED2("Failed to ensure schema");
    }
}

void PaymentDB::writePayment(
    const std::string& from,
    const std::string& to,
    const std::string& value,
    const std::string& nonce,
    const std::string& resourceHash,
    uint64_t timestamp,
    const std::string& transactionHash,
    const std::string& jsonInfo)
{
    try {
        soci::session sql(soci::sqlite3, dbPath_);
        sql << "INSERT INTO payments (from, to, value, nonce, hash, timestamp, transactionHash, jsonInfo) "
               "VALUES (:from, :to, :value, :nonce, :hash, :timestamp, :transactionHash, :jsonInfo)",
            soci::use(from),
            soci::use(to),
            soci::use(value),
            soci::use(nonce),
            soci::use(resourceHash),
            soci::use(timestamp),
            soci::use(transactionHash),
            soci::use(jsonInfo);
    } catch(...) {
        RETHROW_NESTED2("Failed to write payment");
    }
}