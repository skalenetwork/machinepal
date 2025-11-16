#include "MachinePayCommon.h"
#include "FacilitatorClient.h"


#include "exceptions/BadGatewayException.h"
#include "exceptions/ForbiddenException.h"
#include "exceptions/GatewayTimeoutException.h"
#include "exceptions/NotFoundException.h"
#include "exceptions/ServiceUnavailableException.h"
#include "exceptions/TooManyRequestsException.h"
#include "exceptions/UnauthorizedException.h"
#include "exceptions/UnknownServerErrorException.h"
#include "exceptions/VerificationError.h"


FacilitatorClient::~FacilitatorClient() = default;


FacilitatorClient::FacilitatorClient(
    std::string _base_url, std::string _auth, long _connect_timeout_ms, long _total_timeout_ms )
    : baseUrl_(  _base_url  ),
      authHeaderValue_(  _auth  ),
      connectTimeoutMs_( _connect_timeout_ms ),
      totalTimeoutMs_( _total_timeout_ms ) {
}


std::string FacilitatorClient::extractCBInvalidReason( std::string& _responseData ) const {
    try {
        auto errJson = nlohmann::json::parse( _responseData );
        if ( errJson.contains( "reason" ) && errJson["reason"].is_string() ) {
            return errJson["reason"].get< std::string >();
        }
    } catch ( ... ) {
        // Ignore JSON parse errors here
    }
    return {};
}

void FacilitatorClient::checkForGenericHttpError(
    const std::string url, std::string payload, std::string responseData, long httpCode ) const {
    if ( httpCode < 200 || httpCode >= 300 ) {
        std::string errorExplanationForUser;
        switch ( httpCode ) {
        case 400: {
            errorExplanationForUser = "Bad Request";
            std::string invalidReason = extractCBInvalidReason( responseData );
            errorExplanationForUser += ":" + invalidReason;
            break;
        }
        case 401:
            errorExplanationForUser = "Unauthorized";
            break;
        case 403:
            errorExplanationForUser = "Forbidden";
            break;
        case 404:
            errorExplanationForUser = "Not Found";
            break;
        case 429:
            errorExplanationForUser = "Too Many Requests";
            break;
        case 500:
            errorExplanationForUser = "Internal Server Error";
            break;
        case 502:
            errorExplanationForUser = "Bad Gateway";
            break;
        case 503:
            errorExplanationForUser = "Service Unavailable";
            break;
        case 504:
            errorExplanationForUser = "Gateway Timeout";
            break;
        default:
            errorExplanationForUser = "Unknown Error";
            break;
        }

        std::string errorString =
            ( "HTTP " + std::to_string( httpCode ) + " (" + errorExplanationForUser +
                ") error at " + url + ": " + responseData + "\n | Payload: " + payload );

        LOG( ERROR ) << errorString;

        switch ( httpCode ) {
        case 401:
            throw UnauthorizedException( errorExplanationForUser );
        case 403:
            throw ForbiddenException( errorExplanationForUser );
        case 404:
            throw NotFoundException( errorExplanationForUser );
        case 429:
            throw TooManyRequestsException( errorExplanationForUser );
        case 500:
            throw UnknownServerErrorException( errorExplanationForUser );
        case 502:
            throw BadGatewayException( errorExplanationForUser );
        case 503:
            throw ServiceUnavailableException( errorExplanationForUser );
        case 504:
            throw GatewayTimeoutException( errorExplanationForUser );
        default:
            throw UnknownServerErrorException( errorExplanationForUser );
        }
    }
}

size_t FacilitatorClient::writeCallback(
    char* _ptr, size_t _size, size_t _nmemb, void* _userdata ) {
    const size_t real_size = _size * _nmemb;
    auto* buf = static_cast< std::string* >( _userdata );
    buf->append( _ptr, real_size );
    return real_size;
}

std::string FacilitatorClient::joinUrl( const std::string& _base, const std::string& _path ) {
    if ( _base.empty() )
        return _path;
    if ( _path.empty() )
        return _base;
    const bool b = _base.back() == '/';
    const bool p = _path.front() == '/';
    if ( b && p )
        return _base + _path.substr( 1 );
    if ( !b && !p )
        return _base + "/" + _path;
    return _base + _path;
}