#pragma once

#include <nlohmann/json.hpp>

// Abstract interface for facilitator clients capable of verifying and settling payments.
// This allows different concrete facilitator backends (e.g., Coinbase, mock, etc.).
class FacilitatorClient {
public:
    virtual ~FacilitatorClient();

    // POST /verify — validates the payment payload (no chain call)
    virtual nlohmann::json verify(const nlohmann::json& paymentInstruction,
                                  const nlohmann::json& paymentPayload) const = 0;

    // POST /settle — performs the on-chain transfer (gas sponsored by facilitator)
    virtual nlohmann::json settle(const nlohmann::json& paymentInstruction,
                                  const nlohmann::json& paymentPayload) const = 0;
};
