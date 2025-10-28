#include "MachinePayCommon.h"
#include "MachinePayDB.h"
#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/postgresql/soci-postgresql.h>
#include <filesystem>
#include <stdexcept> // For std::runtime_error
#include <memory>    // For std::make_unique
#include <spdlog/sinks/stdout_sinks.h>

#include "PaymentRecord.h"
#include "MachinePayApp.h"

using namespace std;


void MachinePayDB::checkSqlLiteFileOnDisk() {
    if (std::filesystem::exists(connectionString_)) {
        if (std::filesystem::is_directory(connectionString_)) {
            throw std::runtime_error("SQLite database path is a directory, not a file: " + connectionString_);
        }
        std::fstream file(connectionString_, std::ios::in | std::ios::out);
        if (!file.is_open()) {
            throw std::runtime_error(
                "Permissions problem: cannot open SQLite database file for read/write: " +
                connectionString_);
        }
        file.close();
    } else {
        spdlog::info("SQLite database file does not exist at {}, it will be created.", connectionString_);
    }
}

void MachinePayDB::verifyDatabaseConnectivity() {
    try {
        if (dbType_ == DbType::SQLite) {
            checkSqlLiteFileOnDisk();
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

void MachinePayDB::configureDBParamsAndPool() {
    const int POOL_SIZE = 8;
    soci::backend_factory const &backend = getBackend(dbType_);
    pool_ = std::make_unique<soci::connection_pool>(POOL_SIZE);
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        soci::session &sess = pool_->at(i);
        sess.open(backend, connectionString_);
        if (dbType_ == DbType::SQLite) {
            sess << "PRAGMA journal_mode=WAL"; // apply to every pooled connection
        }
    }
}

/**
 * @brief Constructs the PaymentDB.
 */
MachinePayDB::MachinePayDB(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo)
    : app_(app),
      dbType_(type) {
    try {
        logger_ = spdlog::get("machinepay.db");
        if (!logger_) {
            logger_ = spdlog::stderr_logger_mt("machinepay.db");
        }
        CHECK_STATE(logger_);

        if (dbType_ == DbType::SQLite) {
            // Ensure config directory exists, then build DB file path
            auto dataDir = app_.configPath() / "data";
            std::filesystem::create_directories(dataDir);
            connectionString_ = (dataDir / "machinepay.db").string();
      } else {
            CHECK_STATE(connectionInfo);
            connectionString_ = connectionInfo.value();
        }

        verifyDatabaseConnectivity();
        configureDBParamsAndPool();
        ensureSchema();
    } catch (...) {
        RETHROW_NESTED2("Failed to initialize PaymentDB");
    }
}

/**
 * @brief Writes a payment record to the database.
 */
/**
 * @brief Writes a payment record to the database.
 */
void MachinePayDB::writePayment(const PaymentRecord &record) {
    try {
        soci::session sql(*pool_);

        // Store record fields in local variables
        std::string fromAddress = record.fromAddress().toHex();
        std::string toAddress = record.toAddress().toHex();
        std::string value = record.value().toDecimal();
        std::string nonce = record.nonce().toHex();
        std::string resourceHash = Encoding::hashToHex(record.resourceHash());
        uint64_t timestamp = record.timestamp();
        std::string authorizationHash = Encoding::hashToHex(record.authorizationHash());
        std::string transactionHash = Encoding::hashToHex(record.transactionHash());
        std::string jsonInfo = record.jsonInfo();

        // Insert into the database using explicit named bindings for safety and cross-backend consistency
        sql << R"(
            INSERT INTO payments (
                fromAddress, toAddress, value, nonce, resourceHash,
                timestamp, authorizationHash, transactionHash, jsonInfo
            )
            VALUES (
                :fromAddress, :toAddress, :value, :nonce, :resourceHash,
                :timestamp, :authorizationHash, :transactionHash, :jsonInfo
            )
        )",
        soci::use(fromAddress, "fromAddress"),
        soci::use(toAddress, "toAddress"),
        soci::use(value, "value"),
        soci::use(nonce, "nonce"),
        soci::use(resourceHash, "resourceHash"),
        soci::use(timestamp, "timestamp"),
        soci::use(authorizationHash, "authorizationHash"),
        soci::use(transactionHash, "transactionHash"),
        soci::use(jsonInfo, "jsonInfo");
    } catch (...) {
        RETHROW_NESTED2("Failed to write payment");
    }
}



// --- Private Helpers ---

/**
 * @brief Gets the app`ropriate SOCI backend factory based on the DbType.
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
                    "resourceHash TEXT NOT NULL,"
                    "timestamp INTEGER NOT NULL," // SQLite's INTEGER handles 64-bit
                    "authorizationHash  TEXT NOT NULL,"
                    "transactionHash  TEXT NOT NULL,"
                    "jsonInfo TEXT)";
        } else if (dbType_ == DbType::PostgreSQL) {
            sql << "CREATE TABLE IF NOT EXISTS payments ("
                    "id SERIAL PRIMARY KEY," // PostgreSQL uses SERIAL
                    "fromAddress TEXT NOT NULL," // Renamed from 'from'
                    "toAddress TEXT NOT NULL," // Renamed from 'to'
                    "value TEXT NOT NULL,"
                    "nonce TEXT NOT NULL,"
                    "resourceHash TEXT NOT NULL,"
                    "timestamp BIGINT NOT NULL," // PostgreSQL uses BIGINT for 64-bit
                    "transactionHash  TEXT NOT NULL,"
                    "authorizationHash  TEXT NOT NULL,"
                    "jsonInfo TEXT)";
        }

        // Index creation
        // Use a UNIQUE index on 'hash' for data integrity
        sql << "CREATE UNIQUE INDEX IF NOT EXISTS idx_payments_reshash ON payments(resourceHash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_payments_authhash ON payments(authorizationHash)";
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
