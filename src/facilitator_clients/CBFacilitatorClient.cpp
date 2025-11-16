#include "CBFacilitatorClient.h"
#include "MachinePayCommon.h"



#include "exceptions/BadGatewayException.h"
#include "exceptions/ForbiddenException.h"
#include "exceptions/GatewayTimeoutException.h"
#include "exceptions/NotFoundException.h"
#include "exceptions/ServiceUnavailableException.h"
#include "exceptions/TooManyRequestsException.h"
#include "exceptions/UnauthorizedException.h"
#include "exceptions/UnknownServerErrorException.h"
#include "exceptions/VerificationError.h"

CBFacilitatorClient::CBFacilitatorClient(
    std::string _base_url, std::string _auth, long _connect_timeout_ms, long _total_timeout_ms )
    : FacilitatorClient(_base_url,  _auth, _connect_timeout_ms , _total_timeout_ms ) {
}




const std::string USDC_SEPOLIA_ADDRESS = "0x036CbD53842c5426634e7929541eC2318f3dCF7e";

const std::string VERIFY_PAYLOAD_EXAMPLE = R"JSON(
{
    "x402Version": 1,
    "paymentPayload": {
        "x402Version": 1,
        "scheme": "exact",
        "network": "base-sepolia",
        "payload": {
            "signature": "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef1b",
            "authorization": {
                "from": "0x1111111111111111111111111111111111111111",
                "to": "0x2222222222222222222222222222222222222222",
                "value": "1000",
                "validAfter": "1716150000",
                "validBefore": "1716153600",
                "nonce": "0x1234567890abcdef"
            }
        }
    },
    "paymentRequirements": {
        "scheme": "exact",
        "network": "base-sepolia",
        "maxAmountRequired": "1000",
        "resource": "https://api.example.com/premium/data",
        "description": "Test API data",
        "mimeType": "application/json",
        "payTo": "0x2222222222222222222222222222222222222222",
        "maxTimeoutSeconds": 10,
        "asset": "0x036CbD53842c5426634e7929541eC2318f3dCF7e"
    }
}
)JSON";


nlohmann::json CBFacilitatorClient::verify(
    const nlohmann::json&  verifyRequest) const {
    return postJson( "/verify", verifyRequest );
}

nlohmann::json CBFacilitatorClient::settle(
        const nlohmann::json&  settleRequest
    ) const {

    return postJson( "/settle", settleRequest);
}


nlohmann::json CBFacilitatorClient::postJson(
    const std::string& _path, const nlohmann::json& _body ) const {
    const std::string url = joinUrl( baseUrl_, _path );
    std::string payload = _body.dump();

    payload = VERIFY_PAYLOAD_EXAMPLE;

    CURL* curl = curl_easy_init();
    if ( !curl )
        throw std::runtime_error( "Failed to init CURL easy handle" );

    std::string responseData;
    long httpCode = 0;

    // Build headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append( headers, "Content-Type: application/json" );
    if ( !authHeaderValue_.empty() ) {
        std::string auth = "Authorization: " + authHeaderValue_;
        headers = curl_slist_append( headers, auth.c_str() );
    }
    for ( const auto& h : extraHeaders_ ) {
        headers = curl_slist_append( headers, h.c_str() );
    }

    // Set options
    curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl, CURLOPT_POST, 1L );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, payload.c_str() );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, payload.size() );
    curl_easy_setopt( curl, CURLOPT_USERAGENT, "CBFacilitatorClient/1.0" );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &CBFacilitatorClient::writeCallback );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &responseData );
    curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT_MS, connectTimeoutMs_ );
    curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, totalTimeoutMs_ );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
    if ( !proxyUrl_.empty() ) {
        curl_easy_setopt( curl, CURLOPT_PROXY, proxyUrl_.c_str() );
    }

    // Perform
    CURLcode res = curl_easy_perform( curl );

    // Collect HTTP code before cleanup
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &httpCode );

    // Cleanup
    if ( headers )
        curl_slist_free_all( headers );
    curl_easy_cleanup( curl );


    if ( res != CURLE_OK ) {
        throw std::runtime_error( std::string( "CURL error: " ) + curl_easy_strerror( res ) );
    }

    if ( httpCode == 400 ) {
        std::string errorExplanationForUser = "Bad Request";
        std::string invalidReason = extractCBInvalidReason( responseData );
        errorExplanationForUser += ":" + invalidReason;
        throw VerificationError( errorExplanationForUser );
    }

    { checkForGenericHttpError( url, payload, responseData, httpCode ); }

    if ( responseData.empty() ) {
        throw std::runtime_error( "HTTP server at " + url + " returned empty response (HTTP " +
                                  std::to_string( httpCode ) + ")" );
    }

    // Parse response
    nlohmann::json j;
    try {
        j = nlohmann::json::parse( responseData );
    } catch ( const std::exception& e ) {
        throw std::runtime_error( "Failed to parse JSON (HTTP " + std::to_string( httpCode ) +
                                  ") from " + url + ": " + std::string( e.what() ) +
                                  " | Raw: " + responseData );
    }


    return j;
}
