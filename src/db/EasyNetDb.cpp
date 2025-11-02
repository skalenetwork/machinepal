#include "EasyNetDb.h"
#include "MachinePayCommon.h"
#include "crypto/Encoding.h"  // added for u256 conversions
#include <soci/soci.h>
#include <limits>  // for overflow check
#include <chrono>   // added for settlementTime
#include <random>   // added for nonce
#include <sstream>  // added for nonce hex formatting

EasyNetDb::EasyNetDb(
    MachinePayApp& app, DbType type, const std::optional< std::string >& connectionInfo )
    : MachinePayDb( app, type, connectionInfo ) {
    try {
        soci::backend_factory const& backend = getBackend( dbType_ );
        soci::session sql( backend, connectionString_ );

        if ( dbType_ == DbType::SQLite ) {
            sql << "PRAGMA journal_mode=WAL";
        }

        bool stateTableExisted = false;
        bool transactionsTableExisted = false;

        // --- Check if the state table already exists ---
        if ( dbType_ == DbType::SQLite ) {
            int count = 0;
            sql << "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='state'", soci::into( count );
            stateTableExisted = ( count > 0 );
            int tcount = 0;
            sql << "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='transactions'", soci::into( tcount );
            transactionsTableExisted = ( tcount > 0 );
        }  else if ( dbType_ == DbType::PostgreSQL ) {
            std::string regclassResultState;  // will be empty if NULL
            soci::indicator indState = soci::i_ok;
            sql << "SELECT to_regclass('public.state')", soci::into( regclassResultState, indState );
            stateTableExisted = ( indState != soci::i_null && !regclassResultState.empty() );
            std::string regclassResultTx;  // will be empty if NULL
            soci::indicator indTx = soci::i_ok;
            sql << "SELECT to_regclass('public.transactions')", soci::into( regclassResultTx, indTx );
            transactionsTableExisted = ( indTx != soci::i_null && !regclassResultTx.empty() );
        }


        // --- Create state table if needed ---
        if ( dbType_ == DbType::SQLite ) {
            sql << "CREATE TABLE IF NOT EXISTS state ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT," // id only in sqlite flavor
                   "walletAddress TEXT NOT NULL,"
                   "assetAddress TEXT NOT NULL,"
                   "value TEXT NOT NULL" \
                   ")";
        } else if ( dbType_ == DbType::PostgreSQL ) {
            sql << "CREATE TABLE IF NOT EXISTS state ("
                   "walletAddress TEXT NOT NULL,"\
                   "assetAddress TEXT NOT NULL,"\
                   "value TEXT NOT NULL"\
                   ")";
        }

        // --- Create transactions table if needed ---
        if ( dbType_ == DbType::SQLite ) {
            sql << "CREATE TABLE IF NOT EXISTS transactions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
                   "organizationName TEXT NOT NULL,"\
                   "chainId TEXT NOT NULL,"\
                   "fromAddress TEXT NOT NULL,"\
                   "toAddress TEXT NOT NULL,"\
                   "assetAddress TEXT NOT NULL,"\
                   "value TEXT NOT NULL,"\
                   "nonce TEXT NOT NULL,"\
                   "resourceLocation TEXT NOT NULL,"\
                   "settlementTime INTEGER NOT NULL,"\
                   "transactionHash TEXT NOT NULL,"\
                   "fromIpAddress TEXT NOT NULL,"\
                   "jsonInfo TEXT"\
                   ")";
        } else if ( dbType_ == DbType::PostgreSQL ) {
            sql << "CREATE TABLE IF NOT EXISTS transactions ("
                   "id SERIAL PRIMARY KEY,"\
                   "organizationName TEXT NOT NULL,"\
                   "chainId TEXT NOT NULL,"\
                   "fromAddress TEXT NOT NULL,"\
                   "toAddress TEXT NOT NULL,"\
                   "assetAddress TEXT NOT NULL,"\
                   "value TEXT NOT NULL,"\
                   "nonce TEXT NOT NULL,"\
                   "resourceLocation TEXT NOT NULL,"\
                   "settlementTime INTEGER NOT NULL,"
                   "transactionHash TEXT NOT NULL,"\
                   "fromIpAddress TEXT NOT NULL,"\
                   "jsonInfo TEXT"\
                   ")";
        }

        // --- Indices for state table ---
        sql << "CREATE INDEX IF NOT EXISTS idx_state_walletaddress ON state(walletAddress)";
        sql << "CREATE INDEX IF NOT EXISTS idx_state_value ON state(value)";

        // --- Indices for transactions table (small performance improvement) ---
        sql << "CREATE INDEX IF NOT EXISTS idx_transactions_fromAddress ON transactions(fromAddress)";
        sql << "CREATE INDEX IF NOT EXISTS idx_transactions_transactionHash ON transactions(transactionHash)";
        sql << "CREATE INDEX IF NOT EXISTS idx_transactions_nonce ON transactions(nonce)";
        sql << "CREATE INDEX IF NOT EXISTS idx_settlement_time ON transactions(settlementTime)";



        if ( !stateTableExisted ) {
            logger_->info( "New 'state' table created and schema initialized." );
        } else {
            logger_->info( "Database schema verified, 'state' table already exists." );
        }
        if ( !transactionsTableExisted ) {
            logger_->info( "New 'transactions' table created and schema initialized." );
        } else {
            logger_->info( "Database schema verified, 'transactions' table already exists." );
        }
    } catch ( std::exception& e ) {
        RETHROW_NESTED2( "Failed to ensure schema " + std::string( e.what() ) );
    }
}


void EasyNetDb::newWalletUnsafe(
    const EthAddress& walletAddress, const EthAddress& assetAddress, const EIP3009Value& value ) {
    try {
        soci::session databaseSession( *pool_ );

        // Convert to database-ready strings
        std::string walletAddressDatabaseString = walletAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();
        std::string initialValueDecimalString = value.toDbString();

        logger_->trace(
            "newWallet: inserting state row walletAddress={} assetAddress={} initialValue={}",
            walletAddressDatabaseString, assetContractAddressDatabaseString,
            initialValueDecimalString );

        // Insert into state table
        if ( dbType_ == DbType::SQLite ) {
            databaseSession << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use( walletAddressDatabaseString, "walletAddress" ),
                soci::use( assetContractAddressDatabaseString, "assetAddress" ),
                soci::use( initialValueDecimalString, "value" );
        } else if ( dbType_ == DbType::PostgreSQL ) {
            databaseSession << R"(INSERT INTO state (walletAddress, assetAddress, value)
                     VALUES (:walletAddress, :assetAddress, :value))",
                soci::use( walletAddressDatabaseString, "walletAddress" ),
                soci::use( assetContractAddressDatabaseString, "assetAddress" ),
                soci::use( initialValueDecimalString, "value" );
        } else {
            RETHROW_NESTED2( "Unsupported DB type for newWallet()" );
        }
    } catch ( std::exception& e ) {
        RETHROW_NESTED2( "Failed to insert state row:" + std::string( e.what() ) );
    }
}

EasyNetDb::TransferResult EasyNetDb::processTransferRequest( const EthAddress& fromAddress,
    const EthAddress& toAddress, const EthAddress& assetAddress, const EIP3009Value& value ) {
    std::unique_lock<std::shared_mutex> lock(stateMutex_);
    fundUserWalletWithFundsIfNewWalletUnsafe( fromAddress, assetAddress );
    return transferValueUnsafe( fromAddress, toAddress, assetAddress, value );
}

EasyNetDb::TransferResult EasyNetDb::transferValueUnsafe( const EthAddress& fromAddress,
    const EthAddress& toAddress, const EthAddress& assetAddress, const EIP3009Value& value ) {
    // Early no-op success cases
    if ( fromAddress.toDbString() == toAddress.toDbString() ) {
        logger_->trace( "transferValue: fromAddress == toAddress, no-op success" );
        return TransferResult::TransferSuccess;
    }
    if ( value.value() == 0 ) {
        logger_->trace( "transferValue: amount == 0, no-op success" );
        return TransferResult::TransferSuccess;
    }

    try {
        soci::session databaseSession( *pool_ );
        soci::transaction databaseTransactionScope( databaseSession );  // RAII transaction

        std::string fromWalletAddressDatabaseString = fromAddress.toDbString();
        std::string toWalletAddressDatabaseString = toAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();
        u256 transferAmountValue = value.value();

        // 1. Fetch sender balance
        std::string senderBalanceValueStringFromDatabase;
        soci::indicator senderBalanceIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into( senderBalanceValueStringFromDatabase, senderBalanceIndicator ),
            soci::use( fromWalletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" );

        if ( senderBalanceIndicator == soci::i_null ||
             senderBalanceValueStringFromDatabase.empty() ) {
            RETHROW_NESTED2( "Sender wallet state row not found for walletAddress=" +
                             fromWalletAddressDatabaseString +
                             ", assetAddress=" + assetContractAddressDatabaseString );
        }

        u256 senderCurrentBalanceValue =
            Encoding::u256FromHexOrDecimal( senderBalanceValueStringFromDatabase );


        if ( transferAmountValue > senderCurrentBalanceValue ) {
            logger_->trace(
                "transferValue: insufficient funds walletAddress={} assetAddress={} have={} "
                "need={}",
                fromWalletAddressDatabaseString, assetContractAddressDatabaseString,
                Encoding::u256ToDecimal( senderCurrentBalanceValue ),
                Encoding::u256ToDecimal( transferAmountValue ) );
            return TransferResult::InsufficientFunds;
        }

        // 2. Fetch receiver balance (may be absent)
        std::string receiverBalanceValueStringFromDatabase;
        soci::indicator receiverBalanceIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into( receiverBalanceValueStringFromDatabase, receiverBalanceIndicator ),
            soci::use( toWalletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" );

        u256 receiverCurrentBalanceValue = 0;
        bool receiverStateRowExists = !( receiverBalanceIndicator == soci::i_null ||
                                         receiverBalanceValueStringFromDatabase.empty() );
        if ( receiverStateRowExists ) {
            receiverCurrentBalanceValue =
                Encoding::u256FromHexOrDecimal( receiverBalanceValueStringFromDatabase );
        }

        // Overflow check: receiverCurrentBalanceValue + transferAmountValue must not exceed max
        const u256 maxUint256Value = ( std::numeric_limits< u256 >::max )();
        if ( transferAmountValue > maxUint256Value - receiverCurrentBalanceValue ) {
            logger_->trace(
                "transferValue: overflow would occur receiverWalletAddress={} assetAddress={} "
                "receiverBalance={} "
                "amount={} max={} treating as insufficient funds",
                toWalletAddressDatabaseString, assetContractAddressDatabaseString,
                Encoding::u256ToDecimal( receiverCurrentBalanceValue ),
                Encoding::u256ToDecimal( transferAmountValue ),
                Encoding::u256ToDecimal( maxUint256Value ) );
            return TransferResult::InsufficientFunds;
        }

        // 3. Compute new balances
        u256 senderUpdatedBalanceValueAfterTransfer =
            senderCurrentBalanceValue - transferAmountValue;
        u256 receiverUpdatedBalanceValueAfterTransfer =
            receiverCurrentBalanceValue + transferAmountValue;

        std::string senderUpdatedBalanceDecimalString =
            Encoding::u256ToDecimal( senderUpdatedBalanceValueAfterTransfer );
        std::string receiverUpdatedBalanceDecimalString =
            Encoding::u256ToDecimal( receiverUpdatedBalanceValueAfterTransfer );

        // 4. Persist changes
        databaseSession << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::use( senderUpdatedBalanceDecimalString, "value" ),
            soci::use( fromWalletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" );

        if ( receiverStateRowExists ) {
            databaseSession << "UPDATE state SET value = :value WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
                soci::use( receiverUpdatedBalanceDecimalString, "value" ),
                soci::use( toWalletAddressDatabaseString, "walletAddress" ),
                soci::use( assetContractAddressDatabaseString, "assetAddress" );
        } else {
            databaseSession << "INSERT INTO state (walletAddress, assetAddress, value) VALUES (:walletAddress, :assetAddress, :value)",
                soci::use( toWalletAddressDatabaseString, "walletAddress" ),
                soci::use( assetContractAddressDatabaseString, "assetAddress" ),
                soci::use( receiverUpdatedBalanceDecimalString, "value" );
        }

        logger_->trace(
            "transferValue success: fromWalletAddress={} toWalletAddress={} "
            "assetContractAddress={} transferAmount={} senderUpdatedBalance={} "
            "receiverUpdatedBalance={}",
            fromWalletAddressDatabaseString, toWalletAddressDatabaseString,
            assetContractAddressDatabaseString, Encoding::u256ToDecimal( transferAmountValue ),
            senderUpdatedBalanceDecimalString, receiverUpdatedBalanceDecimalString );

        // --- New: insert transaction record ---
        // TODO: Replace placeholder context fields with real values from your application.
        const std::string organizationName = "defaultOrg";
        const std::string chainId = "testChain";
        const std::string resourceLocation = "";
        const std::string fromIpAddress = "0.0.0.0";
        const std::string jsonInfo = "{}";
        auto transferAmountValueStr = Encoding::u256ToDecimal( transferAmountValue );

        // Generate a simple random hex nonce
        std::stringstream nonceStream;
        nonceStream << std::hex << std::random_device{}() << std::random_device{}();
        const std::string nonce = nonceStream.str();

        // For now reuse nonce as a pseudo transaction hash (replace with real hash if available)
        const std::string transactionHash = nonce;

        // Settlement time = current unix epoch seconds
        auto now = std::chrono::system_clock::now();
        long long settlementTime =
            std::chrono::duration_cast<std::chrono::seconds>( now.time_since_epoch() ).count();

        databaseSession << "INSERT INTO transactions (organizationName, chainId, fromAddress, "
                           "toAddress, assetAddress, value, nonce, resourceLocation, "
                           "settlementTime, transactionHash, fromIpAddress, jsonInfo) "
                           "VALUES (:organizationName, :chainId, :fromAddress, :toAddress, "
                           ":assetAddress, :value, :nonce, :resourceLocation, :settlementTime, "
                           ":transactionHash, :fromIpAddress, :jsonInfo)",
            soci::use( organizationName, "organizationName" ),
            soci::use( chainId, "chainId" ),
            soci::use( fromWalletAddressDatabaseString, "fromAddress" ),
            soci::use( toWalletAddressDatabaseString, "toAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" ),
            soci::use( transferAmountValueStr, "value" ),
            soci::use( nonce, "nonce" ),
            soci::use( resourceLocation, "resourceLocation" ),
            soci::use( settlementTime, "settlementTime" ),
            soci::use( transactionHash, "transactionHash" ),
            soci::use( fromIpAddress, "fromIpAddress" ),
            soci::use( jsonInfo, "jsonInfo" );

        databaseTransactionScope.commit();
        return TransferResult::TransferSuccess;
    } catch ( std::exception& e ) {
        RETHROW_NESTED2( "Failed to transfer value: " + std::string( e.what() ) );
    }
}

void EasyNetDb::fundUserWalletWithFundsIfNewWalletUnsafe(
    const EthAddress& walletAddress, const EthAddress& assetAddress ) {


    try {
        soci::session databaseSession( *pool_ );

        std::string walletAddressDatabaseString = walletAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();

        // Check if the wallet+asset state row already exists.
        std::string existingValueStringFromDatabase;
        soci::indicator existingValueIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND"
                           " assetAddress = :assetAddress",
            soci::into( existingValueStringFromDatabase, existingValueIndicator ),
            soci::use( walletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" );

        if ( !( existingValueIndicator == soci::i_null ||
                 existingValueStringFromDatabase.empty() ) ) {
            logger_->trace(
                "fundUserWalletWithFundsIfNewWallet: wallet already funded walletAddress={} "
                "assetAddress={} existingValue={}",
                walletAddressDatabaseString, assetContractAddressDatabaseString,
                existingValueStringFromDatabase );
            return;  // Already exists; no action.
        }

        // Compute initial funding amount: 1,000,000,000 * 10^18 = 10^27 token units.
        // Use decimal string to build u256 reliably.
        u256 initialFundingAmountValue =
            Encoding::u256FromHexOrDecimal( "1000000000000000000000000000" );  // 1e27
        EIP3009Value initialFundingEip3009Value( initialFundingAmountValue );
        std::string initialFundingDecimalString = initialFundingEip3009Value.toDbString();

        databaseSession << "INSERT INTO state (walletAddress, assetAddress, value) VALUES "
                           "(:walletAddress, :assetAddress, :value)",
            soci::use( walletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" ),
            soci::use( initialFundingDecimalString, "value" );

        logger_->trace(
            "fundUserWalletWithFundsIfNewWallet: funded new wallet walletAddress={} "
            "assetAddress={} initialValue={}",
            walletAddressDatabaseString, assetContractAddressDatabaseString,
            initialFundingDecimalString );
    } catch ( std::exception& e ) {
        RETHROW_NESTED2( "Failed to fund user wallet: " + std::string( e.what() ) );
    }
}

std::optional< u256 > EasyNetDb::getBalance(
    const EthAddress& walletAddress, const EthAddress& assetAddress ) const {
    std::shared_lock<std::shared_mutex> lock(stateMutex_);
    try {
        soci::session databaseSession( *pool_ );
        std::string walletAddressDatabaseString = walletAddress.toDbString();
        std::string assetContractAddressDatabaseString = assetAddress.toDbString();

        std::string balanceValueStringFromDatabase;
        soci::indicator balanceIndicator = soci::i_ok;
        databaseSession << "SELECT value FROM state WHERE walletAddress = :walletAddress AND assetAddress = :assetAddress",
            soci::into( balanceValueStringFromDatabase, balanceIndicator ),
            soci::use( walletAddressDatabaseString, "walletAddress" ),
            soci::use( assetContractAddressDatabaseString, "assetAddress" );

        if ( balanceIndicator == soci::i_null || balanceValueStringFromDatabase.empty() ) {
            return std::nullopt;  // no row
        }
        u256 currentBalanceValue = Encoding::u256FromHexOrDecimal( balanceValueStringFromDatabase );
        return currentBalanceValue;
    } catch ( std::exception& e ) {
        RETHROW_NESTED2( "Failed to read balance: " + std::string( e.what() ) );
    }
}
