#include "MachinePayCommon.h"

#include "VerifyResponse.h"

#include "crypto/Encoding.h"
#include "url/URLUtils.h"

#include <folly/json/json.h>


VerifyResponse::VerifyResponse( bool success, std::optional< std::string > errorReason,
    const std::string& payer,
    const optional< std::string >& originalJson )
    : success_( success ),
      invalidReason( std::move( errorReason ) ),
      payer_( payer ) {
    if ( originalJson ) {
        originalJson_ = originalJson.value();
    } else {
        // construct originalJson_ from the fields
        json j = toJson();
        originalJson_ = j.dump();
    }
}

bool VerifyResponse::success() const {
    return success_;
}
const std::optional< std::string >& VerifyResponse::errorReason() const {
    return invalidReason;
}

const std::string& VerifyResponse::payer() const {
    return payer_;
}

bool VerifyResponse::operator==( const VerifyResponse& other ) const {
    return success_ == other.success_ && invalidReason == other.invalidReason &&
           payer_ == other.payer_;
}

json VerifyResponse::toJson() const {
    json j;
    j["success"] = success_;
    j["payer"] = payer_;
    if ( invalidReason.has_value() ) {
        j["errorReason"] = invalidReason.value();
    }
    return j;
}


ptr< VerifyResponse > VerifyResponse::getErrorVerifyResponse(
    HttpError& error, const std::string& payer ) {
    auto result = make_shared< VerifyResponse >(
        false, error.message(), payer, std::nullopt );
    return result;
}


VerifyResponse VerifyResponse::fromJsonString( std::string const& jsonString ) {
    auto j = json::parse( jsonString );
    bool success = j.at( "success" ).get< bool >();
    std::string transaction = j.at( "transaction" ).get< std::string >();
    std::string network = j.at( "network" ).get< std::string >();
    std::string payer = j.at( "payer" ).get< std::string >();
    std::optional< std::string > errorReason = std::nullopt;
    if ( j.contains( "errorReason" ) && j.at( "errorReason" ).is_string() ) {
        errorReason = j.at( "errorReason" ).get< std::string >();
    }
    return VerifyResponse( success, errorReason,  payer, jsonString );
}

std::string VerifyResponse::originalJsonToBase64() const {
    return Encoding::base64Encode( originalJson_ );
}