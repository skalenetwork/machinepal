
#include "MachinePayCommon.h"
#include "MachinePayDB.h"
#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/postgresql/soci-postgresql.h>
#include <filesystem>
#include <stdexcept> // For std::runtime_error
#include <memory>    // For std::make_unique

#include "MachinePayApp.h"

using namespace std;

// --- Public API ---

/**
 * @brief Constructs the PaymentDB.
 */
MachinePayDB::MachinePayDB(MachinePayApp& app, DbType type, const std::optional<std::string>& connectionInfo)
    : app_(app),
      dbType_(type)
{

    // Define the size of the connection pool
    const int POOL_SIZE = 8;

    try {
        if (dbType_ == DbType::SQLite) {
            // For SQLite, connectionInfo is the data directory.
            connectionString_ = app_.configPath().append("machinepay.db").string();
            std::filesystem::create_directories(connectionString_); // idempotent

        } else {
            // For PostgreSQL, connectionInfo is the full connection string.
            CHECK_STATE(connectionInfo)
            connectionString_ = connectionInfo.value();
        }

        // --- Connection Pool Setup ---
        // 1. Get the correct backend
        soci::backend_factory const& backend = getBackend(dbType_);

        // 2. Create the pool with the correct 1-argument constructor
        pool_ = std::make_unique<soci::connection_pool>(POOL_SIZE);

        // 3. Initialize all connections in the pool using .open()
        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            soci::session& sess = pool_->at(i);
            sess.open(backend, connectionString_);
        }
        // --- End Pool Setup ---

        // Ensure the schema is ready on initialization
        // This will lease one of the new connections
        ensureSchema();
    } catch(...) {
        RETHROW_NESTED2("Failed to initialize PaymentDB");
    }
}

/**
 * @brief Writes a payment record to the database.
 */
void MachinePayDB::writePayment(
    const std::string& fromAddress, // Renamed from 'from'
    const std::string& toAddress,   // Renamed from 'to'
    const std::string& value,
    const std::string& nonce,
    const std::string& resourceHash,
    uint64_t timestamp,
    const std::string& transactionHash,
    const std::string& jsonInfo)
{
    try {
        // Lease a session from the pool.
        // The connection is automatically returned when 'sql' goes out of scope.
        soci::session sql(*pool_);

        // Renamed columns 'fromAddress' and 'toAddress' (no quotes needed)
        // Renamed SOCI parameters ':fromAddress' and ':toAddress'
        sql << "INSERT INTO payments (fromAddress, toAddress, value, nonce, hash, timestamp, transactionHash, jsonInfo) "
               "VALUES (:fromAddress, :toAddress, :value, :nonce, :hash, :timestamp, :transactionHash, :jsonInfo)",
            soci::use(fromAddress),   // Renamed variable
            soci::use(toAddress),     // Renamed variable
            soci::use(value),
            soci::use(nonce),
            soci::use(resourceHash),
            // SOCI can handle uint64_t directly.
            soci::use(timestamp),
            soci::use(transactionHash),
            soci::use(jsonInfo);
    } catch(...) {
        RETHROW_NESTED2("Failed to write payment");
    }
}


// --- Private Helpers ---

/**
 * @brief Gets the appropriate SOCI backend factory based on the DbType.
 */
soci::backend_factory const& MachinePayDB::getBackend(DbType type) {
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
 * @brief Ensures the database schema (tables and indices) exists.
 */
void MachinePayDB::ensureSchema() {
    try {
        // Lease a session from the pool.
        soci::session sql(*pool_);

        // Use conditional DDL for backend-specific syntax
        if (dbType_ == DbType::SQLite) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "fromAddress TEXT NOT NULL," // Renamed from 'from'
                   "toAddress TEXT NOT NULL,"   // Renamed from 'to'
                   "value TEXT NOT NULL,"
                   "nonce TEXT NOT NULL,"
                   "hash TEXT NOT NULL,"
                   "timestamp INTEGER NOT NULL," // SQLite's INTEGER handles 64-bit
                   "transactionHash TEXT NOT NULL,"
                   "jsonInfo TEXT)";
        } else if (dbType_ == DbType::PostgreSQL) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                   "id SERIAL PRIMARY KEY," // PostgreSQL uses SERIAL
                   "fromAddress TEXT NOT NULL," // Renamed from 'from'
                   "toAddress TEXT NOT NULL,"   // Renamed from 'to'
                   "value TEXT NOT NULL,"
                   "nonce TEXT NOT NULL,"
                   "hash TEXT NOT NULL,"
                   "timestamp BIGINT NOT NULL," // PostgreSQL uses BIGINT for 64-bit
                   "transactionHash TEXT NOT NULL,"
                   "jsonInfo TEXT)";
        }

        // Index creation
        // Use a UNIQUE index on 'hash' for data integrity
        sql << "CREATE UNIQUE INDEX IF NOT EXISTS idx_payments_hash ON payments(hash)";
        // Standard indices for common lookups
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_txhash ON payments(transactionHash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_nonce ON payments(nonce)";
    } catch(...) {
        RETHROW_NESTED2("Failed to ensure schema");
    }
}

