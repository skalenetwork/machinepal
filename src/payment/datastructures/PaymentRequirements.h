#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <string>

/**
 * @brief Represents the payment requirements object for an x402 transaction.
 *
 * All string fields use std::string, and the timeout is an integer.
 * Note: maxAmountRequired is stored as a string as it represents a
 * large, unit-less integer (smallest token denomination) which could
 * exceed standard 64-bit integer limits.
 */

/**
 * @brief Represents the full x402 Payment Requirements object,
 * including optional fields like 'outputSchema' and 'extra'.
 */

class OrganizationConfig;
class ResourceConfig;
class MachinePayConfig;

using json = nlohmann::json;

class PaymentRequirements {
public:
    PaymentRequirements( std::string scheme, std::string network, std::string maxAmountRequired,
        std::string resource, std::string description, std::string mimeType,
        std::optional< json > outputSchema, std::string payTo, uint64_t maxTimeoutSeconds,
        std::string asset, json extra ); // moved implementation to .cpp

    bool operator==( const PaymentRequirements& other ) const; // moved implementation to .cpp

    friend std::ostream& operator<<( std::ostream& os, const PaymentRequirements& p ); // moved to .cpp

    // Getters (moved implementations to .cpp)
    const std::string& scheme() const;
    const std::string& network() const;
    const std::string& maxAmountRequired() const;
    const std::string& resource() const;
    const std::string& description() const;
    const std::string& mimeType() const;
    const std::optional< json >& outputSchema() const;
    const std::string& payTo() const;
    uint64_t maxTimeoutSeconds() const;
    const std::string& asset() const;
    const json& extra() const;

    static std::shared_ptr< PaymentRequirements > fromJson( const json& j );

    json toJson() const;

private:
    static std::shared_ptr< std::string > toString( const PaymentRequirements& p );

    std::string scheme_;
    std::string network_;
    std::string maxAmountRequired_;
    std::string resource_;
    std::string description_;
    std::string mimeType_;
    std::optional< json > outputSchema_;
    std::string payTo_;
    uint64_t maxTimeoutSeconds_;
    std::string asset_;
    json extra_;  // New: object | null
};

// Add these declarations for ADL:
void toJson( json& j, const PaymentRequirements& p );
void fromJson( const json& j, PaymentRequirements& p );
