#include "MachinePayCommon.h"
#include "MachinePayDB.h"
#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/postgresql/soci-postgresql.h>
#include <filesystem>
#include <stdexcept> // For std::runtime_error
#include <memory>    // For std::make_unique
#include <spdlog/sinks/stdout_sinks.h>

#include " PaymentRecord.h"
#include "MachinePayApp.h"

using namespace std;


void MachinePayDB::verifyDatabaseConnectivity() {
    try {
        if (dbType_ == DbType::SQLite) {
            if (!std::filesystem::exists(connectionString_)) {
                spdlog::info("SQLite database file does not exist at {}, it will be created.", connectionString_);
                return;
            }
            if (std::filesystem::is_directory(connectionString_)) {
                throw std::runtime_error("SQLite database path is a directory, not a file: " + connectionString_);
            }

            // Try to open the file for read/write to check permissions
            {
                std::fstream file(connectionString_, std::ios::in | std::ios::out);
                if (!file.is_open()) {
                    throw std::runtime_error(
                        "Permissions problem: cannot open SQLite database file for read/write: " + connectionString_);
                }
                file.close();
            }
        }
        soci::backend_factory const &backendTest = getBackend(dbType_);
        soci::session testSess(backendTest, connectionString_);
        // Use a universal, simple query to confirm the connection is "live"
        // This works for PostgreSQL, SQLite, MySQL, etc.
        int one = 0;
        testSess << "SELECT 1", soci::into(one);
        // Your custom check macro (or use a standard assert/exception)
        CHECK_STATE2(one == 1, "Database connectivity check failed: unexpected; result from test query");
    } catch (std::exception const &e) {
        std::string errorMsg = "Initial connectivity check failed: " + std::string(e.what());
        RETHROW_NESTED2(errorMsg); // Or however your framework propagates exceptions
    } catch (...) {
        RETHROW_NESTED2("Initial connectivity check failed with an unknown error");
    }

    std::string backendName = (dbType_ == DbType::SQLite) ? "SQLite (at " + connectionString_ + ")" : "PostgreSQL";
    logger_->info("Database connectivity verified successfully. Using {}.", backendName);
}

/**
 * @brief Constructs the PaymentDB.
 */
MachinePayDB::MachinePayDB(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo)
    : app_(app),
      dbType_(type) {

    logger_ = spdlog::get("machinepay.db");
    if (!logger_) {
        logger_ = spdlog::stderr_logger_mt("machinepay.db");
    }
    CHECK_STATE(logger_);

    const int POOL_SIZE = 8;
    try {
        if (dbType_ == DbType::SQLite) {
            // Ensure config directory exists, then build DB file path
            auto dataDir = app_.configPath().append("data");
            std::filesystem::create_directories(dataDir); // idempotent directory creation
            connectionString_ = dataDir.append("machinepay.db").string();
        } else {
            CHECK_STATE(connectionInfo);
            connectionString_ = connectionInfo.value();
        }


        verifyDatabaseConnectivity();


        soci::backend_factory const &backend = getBackend(dbType_);
        pool_ = std::make_unique<soci::connection_pool>(POOL_SIZE);
        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            soci::session &sess = pool_->at(i);
            sess.open(backend, connectionString_);
            if (dbType_ == DbType::SQLite) {
                sess << "PRAGMA journal_mode=WAL"; // apply to every pooled connection
            }
        }
        ensureSchema();
    } catch (...) {
        RETHROW_NESTED2("Failed to initialize PaymentDB");
    }
}

/**
 * @brief Writes a payment record to the database.
 */
void MachinePayDB::writePayment(const PaymentRecord& record) {
    try {
        soci::session sql(*pool_);
        sql <<
            "INSERT INTO payments (fromAddress, toAddress, value, nonce, hash, timestamp, transactionHash, jsonInfo) "
            "VALUES (:fromAddress, :toAddress, :value, :nonce, :hash, :timestamp, :transactionHash, :jsonInfo)",
            soci::use(record.fromAddress),
            soci::use(record.toAddress),
            soci::use(record.value),
            soci::use(record.nonce),
            soci::use(record.resourceHash),
            soci::use(record.timestamp),
            soci::use(record.transactionHash),
            soci::use(record.jsonInfo);
    } catch(...) {
        RETHROW_NESTED2("Failed to write payment");
    }
}


// --- Private Helpers ---

/**
 * @brief Gets the appropriate SOCI backend factory based on the DbType.
 */
soci::backend_factory const &MachinePayDB::getBackend(DbType type) {
    switch (type) {
        case DbType::SQLite:
            return soci::sqlite3;
        case DbType::PostgreSQL: {
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

        // check if table exists

        bool tableExisted = false;

        // --- Step 1: Check if the table already exists ---
        if (dbType_ == DbType::SQLite) {
            int count = 0;
            // Query the SQLite master table for our table
            sql << "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='payments'", soci::into(count);
            tableExisted = (count > 0);
        } else if (dbType_ == DbType::PostgreSQL) {
            std::string tableName;
            soci::indicator ind;
            // to_regclass('payments') returns NULL if the table does not exist.
            // soci will set the indicator to i_null in that case.
            sql << "SELECT to_regclass('payments')", soci::into(tableName, ind);
            tableExisted = (ind != soci::i_null);
        }


        // Use conditional DDL for backend-specific syntax
        if (dbType_ == DbType::SQLite) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "fromAddress TEXT NOT NULL," // Renamed from 'from'
                    "toAddress TEXT NOT NULL," // Renamed from 'to'
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
                    "toAddress TEXT NOT NULL," // Renamed from 'to'
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

        // --- Step 4: Log based on our check ---
        if (!tableExisted) {
            logger_->info("Machinepay.db: new 'payments' table created and schema initialized.");
        } else {
            logger_->info("Machinepay.db: database schema verified, 'payments' table already exists.");
        }
    } catch (...) {
        RETHROW_NESTED2("Failed to ensure schema");
    }
}
