
#include "FacilitatorProcessor.h"
#include "BackendConnection.h"
#include "IResponseSender.h"
#include "MachinePayApp.h"
#include "MachinePayCommon.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ServerConfig.h"
#include "payment/datastructures/PaymentPayload.h"
#include "payment/datastructures/PaymentRequiredResponse.h"
#include "payment/datastructures/PaymentRequirements.h"
#include "url/URLUtils.h"

#include <folly/json.h>
#include <boost/beast/core/detail/base64.hpp>


FacilitatorProcessor::FacilitatorProcessor( MachinePayApp& app, ptr< IResponseSender >& responseSender )
    : app_( app ), responseSender_( responseSender ) {
    config_ = app_.configManager()->latestConfig();
}




void FacilitatorProcessor::reply502BadGateway( const std::string& message ) {
    sendResponse( { 502, "Bad Gateway" }, STANDARD_HEADERS, message );

    string body = getErrorBody( message );

    sendResponse( { 502, "Bad Gateway" }, STANDARD_HEADERS, body );
    state_ = State::ERROR_SENT;
}

void FacilitatorProcessor::reply200Success( const std::string& settlementInfo, std::string& proxiedBody ) {
    std::vector< std::pair< std::string, std::string > > headers = {
        { "Content-Type", "text/plain" }, { "X-PAYMENT-RESPONSE", settlementInfo }
    };
    sendResponse( { 200, "OK" }, headers, proxiedBody );
    state_ = State::SUCCESS_RESOURCE_SENT;
}






std::string FacilitatorProcessor::getErrorBody( const std::string& message ) {
        nlohmann::json j;
        j["error"] = message;
        return j.dump();
}

void FacilitatorProcessor::reply500InternalError( const std::string& message ) {
    string body = getErrorBody( message );
    sendResponse( { 500, "Server Error" }, STANDARD_HEADERS, body );
    state_ = State::ERROR_SENT;
}


void FacilitatorProcessor::sendResponse( const std::pair< uint16_t, std::string >& statusAndMessage,
    const std::vector< std::pair< std::string, std::string > >& headers, const std::string& body ) {
    if ( state_ == State::ERROR_SENT ) {
        spdlog::info( "Attempted to send response after error response already sent." );
        return;
    }

    if ( state_ == State::SUCCESS_RESOURCE_SENT ) {
        spdlog::info( "Attempted to send response after resource already sent." );
        return;
    }

    if ( state_ == State::SUCCESS_PAYMENT_REQUIRED_SENT ) {
        spdlog::info( "Attempted to send response after payment required already sent." );
        return;
    }

    CHECK_STATE( responseSender_ );
    try {
        responseSender_->sendResponse( statusAndMessage, headers, body );
    } catch ( std::exception& e ) {
        spdlog::error( "Exception while sending response: {}", e.what() );
        // nothing can be done so we consider response as sent
    }
}



void FacilitatorProcessor::onRequestStart(
    const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders ) noexcept {
    try {
        CHECK_STATE( reqHeaders );

    } catch ( std::exception& e ) {
        spdlog::critical( "onRequestStart exception" );
        printNestedException( e );
        reply500InternalError( "Could not process x402 request start." );
    };
}


void FacilitatorProcessor::replyToClientWithError( const HttpError& httpError ) {
    auto httpErrorMessage = httpError.message();
    switch ( httpError.type() ) {
    case ErrorType::ERR_BAD_REQUEST:
        reply500InternalError( httpErrorMessage );
        break;
    case ErrorType::ERR_INTERNAL_SERVER_ERROR:
        reply500InternalError( httpErrorMessage );
        break;
    case ErrorType::ERR_BAD_GATEWAY:
        reply502BadGateway( httpErrorMessage );
        break;
    default:
        // cant happen
        CHECK_STATE( false );
    }
}

void FacilitatorProcessor::onRequestFullyReceived(
    const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders, const string& body ) noexcept {
    try {
        if ( state_ == State::ERROR_SENT )
            return;

    } catch ( std::exception& e ) {
        spdlog::critical( "onRequestCompletion exception" );
        printNestedException( e );
        reply500InternalError( "Could not process x402 request." );
    }
}


void FacilitatorProcessor::onBodySizeIncrease( size_t newSize ) {
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024;  // 128 KB
    spdlog::info( "[onBodySizeIncrease] Request body size increased to {} bytes", newSize );
    if ( newSize > MAX_BODY_SIZE ) {
        reply500InternalError(
            "Request body too large. Maximum allowed is 1MByte. You can increase this limit in "
            "machinepay config if needed." );
    }
}

std::vector< std::pair< std::string, std::string > > FacilitatorProcessor::STANDARD_HEADERS = {
    { "Content-Type", "application/json" }
};