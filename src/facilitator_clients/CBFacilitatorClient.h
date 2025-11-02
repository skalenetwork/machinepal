#pragma once
#include "FacilitatorClient.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/// Minimal client for Coinbase x402 facilitator (REST JSON API).
/// Depends on: libcurl, nlohmann::json
///
///
//        std::string cdp_url = "https://api.cdp.coinbase.com/platform/v2/x402",
//        std::string cdpAuth = "Bearer XYZ", // Replace with real token if needed
///
class CBFacilitatorClient : public FacilitatorClient {
public:
    explicit CBFacilitatorClient( std::string _base_url = "https://x402.org/facilitator/",
        std::string _auth = "", long _connect_timeout_ms = 5000, long _total_timeout_ms = 15000 );


    ~CBFacilitatorClient() override = default;


    // POST /verify — validates the payment payload (no chain call)
    nlohmann::json verify( const nlohmann::json& _settlementRequest) const;


    // POST /verify — validates the payment payload (no chain call)
    nlohmann::json verify( const nlohmann::json& _paymentInstruction,
        const nlohmann::json& _paymentPayload ) const;

    // POST /settle — performs the on-chain transfer (gas sponsored by facilitator)
    nlohmann::json settle( const nlohmann::json& _paymentInstruction,
        const nlohmann::json& _paymentPayload ) const ;

    // POST /settle — performs the on-chain transfer (gas sponsored by facilitator)
    nlohmann::json settle(
        const nlohmann::json& ) const override {
        throw std::runtime_error( "Not implemented: settle with single argument" );
    };


    std::string extractCBInvalidReason( std::string& _responseData ) const;

    void checkForGenericHttpError(
        std::string url, std::string payload, std::string responseData, long httpCode ) const;

private:
    static void ensureCurlGlobalInit();
    static size_t writeCallback( char* _ptr, size_t _size, size_t _nmemb, void* _userdata );
    static std::string joinUrl( const std::string& _base, const std::string& _path );

    nlohmann::json postJson( const std::string& _path, const nlohmann::json& _body ) const;

    std::string base_url;
    std::string authHeaderValue;
    long connect_timeout_ms;
    long total_timeout_ms;
    std::string proxyUrl;
    std::vector< std::string > extraHeaders;
};
