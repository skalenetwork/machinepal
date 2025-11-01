#include "MachinePayCommon.h"
#include "EasyNetDb.h"
#include <soci/soci.h>

EasyNetDb::EasyNetDb(MachinePayApp &app, DbType type, const std::optional<std::string> &connectionInfo)
: MachinePayDb(app, type, connectionInfo) {
    try {
        // Create a single, temporary session just for schema initialization.
        soci::backend_factory const &backend = getBackend(dbType_);
        soci::session sql(backend, connectionString_);

        if (dbType_ == DbType::SQLite) {
            sql << "PRAGMA journal_mode=WAL";
        }

        bool tableExisted = false;

        // --- Step 1: Check if the state table already exists ---
        if (dbType_ == DbType::SQLite) {
            int count = 0;
            sql << "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='state'", soci::into(count);
            tableExisted = (count > 0);
        } else if (dbType_ == DbType::PostgreSQL) {
            std::string regclassResult; // will be empty if NULL
            soci::indicator ind = soci::i_ok;
            sql << "SELECT to_regclass('public.state')", soci::into(regclassResult, ind);
            tableExisted = (ind != soci::i_null);
        }

        // --- Step 2: Create table if needed ---
        if (dbType_ == DbType::SQLite) {
            sql << "CREATE TABLE IF NOT EXISTS state ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "walletAddress TEXT NOT NULL,"
                   "assetAddress TEXT NOT NULL,"
                   "value TEXT NOT NULL"
                 ")"; // no extra )
        } else if (dbType_ == DbType::PostgreSQL) {
            sql << "CREATE TABLE IF NOT EXISTS state ("
                   "walletAddress TEXT NOT NULL,"
                   "assetAddress TEXT NOT NULL,"
                   "value TEXT NOT NULL)";
        }

        // --- Step 3: Indices ---
        sql << "CREATE INDEX IF NOT EXISTS idx_state_walletaddress ON state(walletAddress)";

        // --- Step 4: Log based on our check ---
        if (!tableExisted) {
            logger_->info("New 'state' table created and schema initialized.");
        } else {
            logger_->info("Database schema verified, 'state' table already exists.");
        }
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to ensure schema " + std::string(e.what()));
    }
}


void EasyNetDb::newWallet(const EthAddress &walletAddress, const EthAddress &assetAddress, const EIP3009Value &value) {
    try {
        soci::session sql(*pool_);

        // Convert to database-ready strings
        std::string walletAddressDb = walletAddress.toDbString();
        std::string assetAddressDb = assetAddress.toDbString();
        std::string valueDb = value.toDbString();

        logger_->trace(
            "Inserting state row: walletAddress={}, assetAddress={}, value={}",
            walletAddressDb, assetAddressDb, valueDb);

        // Insert into state table
        if (dbType_ == DbType::SQLite) {
            sql << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use(walletAddressDb, "walletAddress"),
                soci::use(assetAddressDb, "assetAddress"),
                soci::use(valueDb, "value");
        } else if (dbType_ == DbType::PostgreSQL) {
            // PostgreSQL variant (same columns). Consider ON CONFLICT if uniqueness constraints added later.
            sql << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use(walletAddressDb, "walletAddress"),
                soci::use(assetAddressDb, "assetAddress"),
                soci::use(valueDb, "value");
        } else {
            RETHROW_NESTED2("Unsupported DB type for newWallet()");
        }
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to insert state row:" + std::string(e.what()));
    }
}