#include "MachinePayCommon.h"
#include "EasyNetDb.h"
#include <soci/soci.h>
#include "crypto/Encoding.h" // added for u256 conversions

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
    std::unique_lock<std::shared_mutex> lock(stateMutex_); // exclusive lock for write
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

void EasyNetDb::transferValue(const EthAddress &fromAddress,
                              const EthAddress &toAddress,
                              const EthAddress &assetAddress,
                              const EIP3009Value &value) {
    std::unique_lock<std::shared_mutex> lock(stateMutex_); // lock for read-modify-write sequence
    try {

        soci::session sql(*pool_);
        soci::transaction tr(sql); // RAII transaction

        std::string fromAddressDb = fromAddress.toDbString();
        std::string toAddressDb = toAddress.toDbString();
        std::string assetAddressDb = assetAddress.toDbString();
        u256 amount = value.value();

        // 1. Fetch sender balance
        std::string fromValueStr;
        soci::indicator indFrom = soci::i_ok;
        sql << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into(fromValueStr, indFrom),
            soci::use(fromAddressDb, "walletAddress"),
            soci::use(assetAddressDb, "assetAddress");

        if (indFrom == soci::i_null || fromValueStr.empty()) {
            RETHROW_NESTED2("Sender wallet state row not found for walletAddress=" + fromAddressDb + ", assetAddress=" + assetAddressDb);
        }

        u256 fromBalance = Encoding::u256FromHexOrDecimal(fromValueStr);
        if (amount > fromBalance) {
            RETHROW_NESTED2("Insufficient balance: have=" + Encoding::u256ToDecimal(fromBalance) + ", need=" + Encoding::u256ToDecimal(amount));
        }

        // 2. Fetch receiver balance (may be absent)
        std::string toValueStr;
        soci::indicator indTo = soci::i_ok;
        sql << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into(toValueStr, indTo),
            soci::use(toAddressDb, "walletAddress"),
            soci::use(assetAddressDb, "assetAddress");

        u256 toBalance = 0;
        bool toExists = !(indTo == soci::i_null || toValueStr.empty());
        if (toExists) {
            toBalance = Encoding::u256FromHexOrDecimal(toValueStr);
        }

        // 3. Compute new balances
        u256 newFromBalance = fromBalance - amount;
        u256 newToBalance = toBalance + amount;

        std::string newFromStr = Encoding::u256ToDecimal(newFromBalance);
        std::string newToStr = Encoding::u256ToDecimal(newToBalance);

        // 4. Persist changes
        sql << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::use(newFromStr, "value"),
            soci::use(fromAddressDb, "walletAddress"),
            soci::use(assetAddressDb, "assetAddress");

        if (toExists) {
            sql << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
                soci::use(newToStr, "value"),
                soci::use(toAddressDb, "walletAddress"),
                soci::use(assetAddressDb, "assetAddress");
        } else {
            sql << "INSERT INTO state (walletAddress, assetAddress, value) VALUES (:walletAddress, :assetAddress, :value)",
                soci::use(toAddressDb, "walletAddress"),
                soci::use(assetAddressDb, "assetAddress"),
                soci::use(newToStr, "value");
        }

        logger_->trace(
            "transferValue: from={} to={} asset={} amount={} newFromBalance={} newToBalance={}",
            fromAddressDb, toAddressDb, assetAddressDb,
            Encoding::u256ToDecimal(amount), newFromStr, newToStr);

        tr.commit();
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to transfer value: " + std::string(e.what()));
    }
}