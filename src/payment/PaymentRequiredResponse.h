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
        int x402Version,
        std::vector<PaymentRequirements> accepts
    ) : x402Version_(x402Version),
        accepts_(std::move(accepts)) {
        CHECK_STATE(accepts.size() > 0)
    }
    PaymentRequiredResponse(
        int x402Version,
        std::vector<PaymentRequirements> accepts,
        std::optional<std::string> error
    ) : x402Version_(x402Version),
        accepts_(std::move(accepts)),
        error_(std::move(error)) {
        // Accept either non-empty accepts or an error message explaining why accepts may be empty
        CHECK_STATE(!accepts_.empty() || error_.has_value())
    }

    // Getters
    int x402Version() const { return x402Version_; }
    const std::vector<PaymentRequirements>& accepts() const { return accepts_; }
    const std::optional<std::string>& error() const { return error_; }

    // Equality and stream output for convenience/testing
    bool operator==(const PaymentRequiredResponse& other) const {
        return x402Version_ == other.x402Version_ &&
               accepts_ == other.accepts_ &&
               error_ == other.error_;
    }
    friend std::ostream& operator<<(std::ostream& os, const PaymentRequiredResponse& r) {
        os << "{x402Version: " << r.x402Version_
           << ", accepts: [";
        for (size_t i = 0; i < r.accepts_.size(); ++i) {
            if (i) os << ", ";
            os << r.accepts_[i];
        }
        os << "]";
        os << ", error: " << (r.error_ ? ('"' + *r.error_ + '"') : std::string("null"))
           << "}";
        return os;
    }

    // JSON serialization/deserialization
    static PaymentRequiredResponse fromJson(const json& j);
    json toJson() const;

private:
    uint32_t x402Version_ {1};
    std::vector<PaymentRequirements> accepts_;
    std::optional<std::string> error_ {std::nullopt};
};