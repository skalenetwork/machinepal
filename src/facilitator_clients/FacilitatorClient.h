#pragma once


// Abstract interface for facilitator clients capable of verifying and settling payments.
// This allows different concrete facilitator backends (e.g., Coinbase, mock, etc.).
class FacilitatorClient {
public:
    virtual ~FacilitatorClient();
    FacilitatorClient( std::string _base_url, std::string _auth, long _connect_timeout_ms,
        long _total_timeout_ms );

    /*
    // POST /verify — validates the payment payload (no chain call)
    virtual nlohmann::json verify(
        const nlohmann::json& verifyRequestJson) const = 0;

    // POST /settle — performs the on-chain transfer (gas sponsored by facilitator)
    virtual nlohmann::json settle(
        const nlohmann::json& settlementRequestJson) const = 0;
        */

    std::string baseUrl_;
    std::string authHeaderValue_;
    long connectTimeoutMs_;
    long totalTimeoutMs_;
    std::string proxyUrl_;
    std::vector< std::string > extraHeaders_;

};
