#include "PaymentDB.h"
#include "MachinePayCommon.h" // Assumed to provide RETHROW_NESTED2

#include <soci/sqlite3/soci-sqlite3.h>
#ifdef ENABLE_POSTGRESQL
#include <soci/postgresql/soci-postgresql.h>
#endif
#include <filesystem>
#include <stdexcept> // For std::runtime_error

using namespace std;

/**
 * @brief Gets the appropriate SOCI backend factory based on the DbType.
 */
soci::backend_factory const& PaymentDB::getBackend(DbType type) {
    switch (type) {
        case DbType::SQLite:
            return soci::sqlite3;
        case DbType::PostgreSQL:
        {
        #ifdef ENABLE_POSTGRESQL
            return soci::postgresql;
        #else
            throw std::runtime_error("PostgreSQL backend not enabled at build time");
        #endif
        }
        default:
            // This should not be reachable if all enum values are handled
            throw std::runtime_error("Unsupported database type");
    }
}

/**
 * @brief Constructs the PaymentDB.
 */
PaymentDB::PaymentDB(MachinePayApp& app, DbType type, const std::string& connectionInfo)
    : app_(app),
      dbType_(type),
      backend_(getBackend(type))
{
    try {
        if (dbType_ == DbType::SQLite) {
            // For SQLite, connectionInfo is the data directory.
            // We create it and append the standard database filename.
            std::filesystem::create_directories(connectionInfo); // idempotent
            connectionString_ = connectionInfo + "/machinepay.db";
        } else {
            // For PostgreSQL, connectionInfo is the full connection string.
            connectionString_ = connectionInfo;
        }

        // Ensure the schema is ready on initialization
        ensureSchema();
    } catch(...) {
        RETHROW_NESTED2("Failed to initialize PaymentDB");
    }
}

/**
 * @brief Creates and returns a new SOCI session.
 */
soci::session PaymentDB::getSession() {
    // This now uses the stored backend and connection string
    return soci::session(backend_, connectionString_);
}

/**
 * @brief Ensures the database schema (tables and indices) exists.
 */
void PaymentDB::ensureSchema() {
    try {
        soci::session sql = getSession();

        // Use conditional DDL for backend-specific syntax
        if (dbType_ == DbType::SQLite) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "\"from\" TEXT NOT NULL," // Quoted for safety (SQL keyword)
                   "\"to\" TEXT NOT NULL,"   // Quoted for safety (SQL keyword)
                   "value TEXT NOT NULL,"
                   "nonce TEXT NOT NULL,"
                   "hash TEXT NOT NULL,"
                   "timestamp INTEGER NOT NULL," // SQLite's INTEGER handles 64-bit
                   "transactionHash TEXT NOT NULL,"
                   "jsonInfo TEXT)";
        } else if (dbType_ == DbType::PostgreSQL) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                   "id SERIAL PRIMARY KEY," // PostgreSQL uses SERIAL
                   "\"from\" TEXT NOT NULL,"
                   "\"to\" TEXT NOT NULL,"
                   "value TEXT NOT NULL,"
                   "nonce TEXT NOT NULL,"
                   "hash TEXT NOT NULL,"
                   "timestamp BIGINT NOT NULL," // PostgreSQL uses BIGINT for 64-bit
                   "transactionHash TEXT NOT NULL,"
                   "jsonInfo TEXT)";
        }

        // Index creation syntax is compatible across both backends
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_hash ON payments(hash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_txhash ON payments(transactionHash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_nonce ON payments(nonce)";
    } catch(...) {
        RETHROW_NESTED2("Failed to ensure schema");
    }
}

/**
 * @brief Writes a payment record to the database.
 */
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
        soci::session sql = getSession();

        // The INSERT statement is standard SQL.
        // Quoted "from" and "to" to avoid conflicts with SQL keywords.
        sql << "INSERT INTO payments (\"from\", \"to\", value, nonce, hash, timestamp, transactionHash, jsonInfo) "
               "VALUES (:from, :to, :value, :nonce, :hash, :timestamp, :transactionHash, :jsonInfo)",
            soci::use(from),
            soci::use(to),
            soci::use(value),
            soci::use(nonce),
            soci::use(resourceHash),
            soci::use(timestamp), // SOCI handles uint64_t -> INTEGER/BIGINT
            soci::use(transactionHash),
            soci::use(jsonInfo);
    } catch(...) {
        RETHROW_NESTED2("Failed to write payment");
    }
}
