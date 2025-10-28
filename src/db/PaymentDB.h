#pragma once

#include <soci/soci.h>
#include <soci/connection-pool.h>
#include <string>
#include <memory>

// Forward-declare MachinePayApp to avoid circular include
class MachinePayApp;

/**
 * @brief Defines the supported database backend types.
 */
enum class DbType {
    SQLite,
    PostgreSQL
};

/**
 * @brief Manages database operations for payments using SOCI.
 * This class is now thread-safe due to the use of soci::connection_pool.
 */
class PaymentDB {
public:
    /**
     * @brief Constructs the PaymentDB and initializes the connection pool.
     * @param app Reference to the main application class.
     * @param type The database backend to use (SQLite or PostgreSQL).
     * @param connectionInfo For SQLite: the directory path. For PostgreSQL: the full connection string.
     */
    PaymentDB(MachinePayApp& app, DbType type, const std::string& connectionInfo);

    /**
     * @brief Writes a payment record to the database.
     * This method is thread-safe.
     */
    void writePayment(
        const std::string& fromAddress, // Renamed from 'from'
        const std::string& toAddress,   // Renamed from 'to'
        const std::string& value,
        const std::string& nonce,
        const std::string& resourceHash,
        uint64_t timestamp,
        const std::string& transactionHash,
        const std::string& jsonInfo);

private:
    /**
     * @brief Gets the appropriate SOCI backend factory based on the DbType.
     */
    soci::backend_factory const& getBackend(DbType type);

    /**
     * @brief Ensures the database schema (tables and indices) exists.
     */
    void ensureSchema();

    // Member variables
    MachinePayApp& app_;
    DbType dbType_;
    std::string connectionString_;

    /**
     * @brief Thread-safe connection pool.
     */
    std::unique_ptr<soci::connection_pool> pool_;
};

