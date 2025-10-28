#pragma once

#include <string>
#include <soci/soci.h>

// Forward declaration to avoid including the full app header
class MachinePayApp;

/**
 * @brief Enum to specify the database backend type.
 */
enum class DbType {
    SQLite,
    PostgreSQL
};

/**
 * @brief Handles database operations for payments, supporting multiple backends.
 */
class PaymentDB {
public:
    /**
     * @brief Constructs the PaymentDB.
     * @param app The main application reference.
     * @param type The database backend to use (SQLite or PostgreSQL).
     * @param connectionInfo For SQLite, this is the data directory. For PostgreSQL,
     * this is the full SOCI connection string
     * (e.g., "dbname=machinepay user=postgres password=secret").
     */
    PaymentDB(MachinePayApp& app, DbType type, const std::string& connectionInfo);

    /**
     * @brief Writes a payment record to the database.
     */
    void writePayment(
        const std::string& from,
        const std::string& to,
        const std::string& value,
        const std::string& nonce,
        const std::string& resourceHash,
        uint64_t timestamp,
        const std::string& transactionHash,
        const std::string& jsonInfo
    );

private:
    /**
     * @brief Ensures the database schema (tables and indices) exists.
     */
    void ensureSchema();

    /**
     * @brief Creates and returns a new SOCI session.
     * @return A soci::session connected to the configured database.
     */
    soci::session getSession();

    /**
     * @brief Gets the appropriate SOCI backend factory based on the DbType.
     * @param type The database type.
     * @return A constant reference to the SOCI backend factory.
     */
    soci::backend_factory const& getBackend(DbType type);

    MachinePayApp& app_;
    DbType dbType_;
    std::string connectionString_;
    soci::backend_factory const& backend_;
};
