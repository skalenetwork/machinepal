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
        sql << "CREATE INDEX IF NOT EXISTS idx_state_value ON state(value)"; // added index by value

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
    std::unique_lock<std::shared_mutex> stateMutexUniqueLock(stateMutex_); // exclusive lock for write
    try {
        soci::session databaseSession(*pool_);

        // Convert to database-ready strings
        std::string walletAddressDatabaseString = walletAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();
        std::string initialValueDecimalString = value.toDbString();

        logger_->trace(
            "newWallet: inserting state row walletAddress={} assetAddress={} initialValue={}",
            walletAddressDatabaseString, assetContractAddressDatabaseString, initialValueDecimalString);

        // Insert into state table
        if (dbType_ == DbType::SQLite) {
            databaseSession << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use(walletAddressDatabaseString, "walletAddress"),
                soci::use(assetContractAddressDatabaseString, "assetAddress"),
                soci::use(initialValueDecimalString, "value");
        } else if (dbType_ == DbType::PostgreSQL) {
            databaseSession << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use(walletAddressDatabaseString, "walletAddress"),
                soci::use(assetContractAddressDatabaseString, "assetAddress"),
                soci::use(initialValueDecimalString, "value");
        } else {
            RETHROW_NESTED2("Unsupported DB type for newWallet()");
        }
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to insert state row:" + std::string(e.what()));
    }
}

void EasyNetDb::processTransferRequest(const EthAddress &fromAddress,
                              const EthAddress &toAddress,
                              const EthAddress &assetAddress,
                              const EIP3009Value &value) {
    fundUserWalletWithFundsIfNewWallet(fromAddress, assetAddress);
    transferValue(fromAddress, toAddress, assetAddress, value);
}

void EasyNetDb::transferValue(const EthAddress &fromAddress,
                              const EthAddress &toAddress,
                              const EthAddress &assetAddress,
                              const EIP3009Value &value) {
    std::unique_lock<std::shared_mutex> stateMutexUniqueLock(stateMutex_); // lock for read-modify-write sequence
    try {
        soci::session databaseSession(*pool_);
        soci::transaction databaseTransactionScope(databaseSession); // RAII transaction

        std::string fromWalletAddressDatabaseString = fromAddress.toDbString();
        std::string toWalletAddressAddressDatabaseString = toAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();
        u256 transferAmountValue = value.value();

        // 1. Fetch sender balance
        std::string senderBalanceValueStringFromDatabase;
        soci::indicator senderBalanceIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into(senderBalanceValueStringFromDatabase, senderBalanceIndicator),
            soci::use(fromWalletAddressDatabaseString, "walletAddress"),
            soci::use(assetContractAddressDatabaseString, "assetAddress");

        if (senderBalanceIndicator == soci::i_null || senderBalanceValueStringFromDatabase.empty()) {
            RETHROW_NESTED2("Sender wallet state row not found for walletAddress=" + fromWalletAddressDatabaseString + ", assetAddress=" + assetContractAddressDatabaseString);
        }

        u256 senderCurrentBalanceValue = Encoding::u256FromHexOrDecimal(senderBalanceValueStringFromDatabase);
        if (transferAmountValue > senderCurrentBalanceValue) {
            RETHROW_NESTED2("Insufficient balance: have=" + Encoding::u256ToDecimal(senderCurrentBalanceValue) + ", need=" + Encoding::u256ToDecimal(transferAmountValue));
        }

        // 2. Fetch receiver balance (may be absent)
        std::string receiverBalanceValueStringFromDatabase;
        soci::indicator receiverBalanceIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into(receiverBalanceValueStringFromDatabase, receiverBalanceIndicator),
            soci::use(toWalletAddressAddressDatabaseString, "walletAddress"),
            soci::use(assetContractAddressDatabaseString, "assetAddress");

        u256 receiverCurrentBalanceValue = 0;
        bool receiverStateRowExists = !(receiverBalanceIndicator == soci::i_null || receiverBalanceValueStringFromDatabase.empty());
        if (receiverStateRowExists) {
            receiverCurrentBalanceValue = Encoding::u256FromHexOrDecimal(receiverBalanceValueStringFromDatabase);
        }

        // 3. Compute new balances
        u256 senderUpdatedBalanceValueAfterTransfer = senderCurrentBalanceValue - transferAmountValue;
        u256 receiverUpdatedBalanceValueAfterTransfer = receiverCurrentBalanceValue + transferAmountValue;

        std::string senderUpdatedBalanceDecimalString = Encoding::u256ToDecimal(senderUpdatedBalanceValueAfterTransfer);
        std::string receiverUpdatedBalanceDecimalString = Encoding::u256ToDecimal(receiverUpdatedBalanceValueAfterTransfer);

        // 4. Persist changes
        databaseSession << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::use(senderUpdatedBalanceDecimalString, "value"),
            soci::use(fromWalletAddressDatabaseString, "walletAddress"),
            soci::use(assetContractAddressDatabaseString, "assetAddress");

        if (receiverStateRowExists) {
            databaseSession << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
                soci::use(receiverUpdatedBalanceDecimalString, "value"),
                soci::use(toWalletAddressAddressDatabaseString, "walletAddress"),
                soci::use(assetContractAddressDatabaseString, "assetAddress");
        } else {
            databaseSession << "INSERT INTO state (walletAddress, assetAddress, value) VALUES (:walletAddress, :assetAddress, :value)",
                soci::use(toWalletAddressAddressDatabaseString, "walletAddress"),
                soci::use(assetContractAddressDatabaseString, "assetAddress"),
                soci::use(receiverUpdatedBalanceDecimalString, "value");
        }

        logger_->trace(
            "transferValue: fromWalletAddress={} toWalletAddress={} assetContractAddress={} transferAmount={} senderUpdatedBalance={} receiverUpdatedBalance={}",
            fromWalletAddressDatabaseString,
            toWalletAddressAddressDatabaseString,
            assetContractAddressDatabaseString,
            Encoding::u256ToDecimal(transferAmountValue),
            senderUpdatedBalanceDecimalString,
            receiverUpdatedBalanceDecimalString);

        databaseTransactionScope.commit();
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to transfer value: " + std::string(e.what()));
    }
}

void EasyNetDb::fundUserWalletWithFundsIfNewWallet(const EthAddress &walletAddress, const EthAddress &assetAddress) {
    std::unique_lock<std::shared_mutex> stateMutexUniqueLock(stateMutex_); // exclusive lock for potential insert
    try {
        soci::session databaseSession(*pool_);

        std::string walletAddressDatabaseString = walletAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();

        // Check if the wallet+asset state row already exists.
        std::string existingValueStringFromDatabase;
        soci::indicator existingValueIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into(existingValueStringFromDatabase, existingValueIndicator),
            soci::use(walletAddressDatabaseString, "walletAddress"),
            soci::use(assetContractAddressDatabaseString, "assetAddress");

        if (!(existingValueIndicator == soci::i_null || existingValueStringFromDatabase.empty())) {
            logger_->trace("fundUserWalletWithFundsIfNewWallet: wallet already funded walletAddress={} assetAddress={} existingValue={}",
                           walletAddressDatabaseString, assetContractAddressDatabaseString, existingValueStringFromDatabase);
            return; // Already exists; no action.
        }

        // Compute initial funding amount: 1,000,000,000 * 10^18 = 10^27 token units.
        // Use decimal string to build u256 reliably.
        u256 initialFundingAmountValue = Encoding::u256FromHexOrDecimal("1000000000000000000000000000"); // 1e27
        EIP3009Value initialFundingEip3009Value(initialFundingAmountValue);
        std::string initialFundingDecimalString = initialFundingEip3009Value.toDbString();

        databaseSession << "INSERT INTO state (walletAddress, assetAddress, value) VALUES (:walletAddress, :assetAddress, :value)",
            soci::use(walletAddressDatabaseString, "walletAddress"),
            soci::use(assetContractAddressDatabaseString, "assetAddress"),
            soci::use(initialFundingDecimalString, "value");

        logger_->trace("fundUserWalletWithFundsIfNewWallet: funded new wallet walletAddress={} assetAddress={} initialValue={}",
                       walletAddressDatabaseString, assetContractAddressDatabaseString, initialFundingDecimalString);
    } catch (std::exception &e) {
        RETHROW_NESTED2("Failed to fund user wallet: " + std::string(e.what()));
    }
}
