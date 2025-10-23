#pragma once

#include <vector>
#include <optional>
#include <string>
#include "nlohmann/json.hpp"
#include "PaymentRequirements.h"
#include <ostream>

/**
 * HTTP 402 response body returned by an x402-enabled server.
 * Example shape:
 * {
 *   "x402Version": 1,
 *   "accepts": [ { ... PaymentRequirements ... } ],
 *   "error": "Optional error message"
 * }
 */
class PaymentRequiredResponse {
public:
    using json = nlohmann::json;

    PaymentRequiredResponse() = default;
    PaymentRequiredResponse(
        std::vector<PaymentRequirements>& accepts
    ) : accepts_(accepts) {
        CHECK_STATE(!accepts.empty());
        error_ = "X-PAYMENT header is required";
    }
    PaymentRequiredResponse(
        std::vector<PaymentRequirements>& accepts,
        optional<std::string>&  error
    ) :
        accepts_(accepts),
        error_(error) {
        // Accept either non-empty accepts or an error message explaining why accepts may be empty
        CHECK_STATE(!accepts_.empty())
    }

    // Getters
    int x402Version() const { return x402Version_; }
    const std::vector<PaymentRequirements>& accepts() const { return accepts_; }
    const optional<string> error() const { return error_; }

    // Equality and stream output for convenience/testing
    bool operator==(const PaymentRequiredResponse& other) const {
        return x402Version_ == other.x402Version_ &&
               accepts_ == other.accepts_ &&
               error_ == other.error_;
    }


    // JSON serialization/deserialization
    static PaymentRequiredResponse fromJson(const json& j);
    json toJson() const;



    static std::string getPaymentRequiredResponseAsString(ptr<OrganizationConfig> organization,
                                                              ptr<ResourceConfig> resource,
                                                              ptr<MachinePayConfig> config);

private:
    uint32_t x402Version_ {1};
    std::vector<PaymentRequirements> accepts_;
    optional<string> error_;
};