
#include "FacilitatorProcessor.h"
#include  "x402_server/X402Handler.h"
#include "IResponseSender.h"
#include "MachinePayApp.h"
#include "MachinePayCommon.h"
#include "config/subconfigs/OrganizationConfig.h"
#include "config/subconfigs/ServerConfig.h"
#include "facilitators/EasyNetFacilitator.h"


auto SETTLE_PATH = EASYNET_FACILITATOR_PREFIX + string("/settle");
auto VERIFY_PATH = EASYNET_FACILITATOR_PREFIX + string("/verify");


FacilitatorProcessor::FacilitatorProcessor( MachinePayApp& app, ptr< IResponseSender >& responseSender )
    : app_( app ), responseSender_( responseSender ) {
    config_ = app_.configManager()->latestConfig();
}


void FacilitatorProcessor::reply400BadRequest( const std::string& message ) {
    string body = getErrorBody( message );
    sendResponse( { 400, "Bad request" }, STANDARD_HEADERS, body );
    state_ = FacilitatorProcessorState::ERROR_SENT;
}

void FacilitatorProcessor::reply500InternalError( const std::string& message ) {
    string body = getErrorBody( message );
    sendResponse( { 500, "Server Error" }, STANDARD_HEADERS, body );
    state_ = FacilitatorProcessorState::ERROR_SENT;
}

void FacilitatorProcessor::reply405MethodNotAllowed( const std::string& message ) {
    string body = getErrorBody( message );
    auto headers = STANDARD_HEADERS;
    headers.push_back( {"Allow", "POST"} );
    sendResponse( { 405, "Method Not Allowed" }, headers, body );
    state_ = FacilitatorProcessorState::ERROR_SENT;
}

void FacilitatorProcessor::reply413PayloadTooLarge( const std::string& message ) {
    string body = getErrorBody( message );
    sendResponse( { 413, "Payload Too Large" }, STANDARD_HEADERS, body );
    state_ = FacilitatorProcessorState::ERROR_SENT;
}

void FacilitatorProcessor::reply415UnsupportedMediaType( const std::string& message ) {
    string body = getErrorBody( message );
    auto headers = STANDARD_HEADERS;
    headers.push_back( {"Accept-Post", "application/json"} );
    sendResponse( { 415, "Unsupported Media Type" }, headers, body );
    state_ = FacilitatorProcessorState::ERROR_SENT;
}




std::string FacilitatorProcessor::getErrorBody( const std::string& message ) {
        nlohmann::json j;
        j["error"] = message;
        return j.dump();
}



void FacilitatorProcessor::sendResponse( const std::pair< uint16_t, std::string >& statusAndMessage,
    const std::vector< std::pair< std::string, std::string > >& headers, const std::string& body ) {
    if ( state_ == FacilitatorProcessorState::ERROR_SENT) {
        spdlog::error( "Attempted to send response after error response already sent." );
        return;
    }

    if ( state_ == FacilitatorProcessorState::SUCCESS_REPLY_SENT ) {
        spdlog::error( "Attempted to send response after resource already sent." );
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
        if (reqHeaders->getMethod() != proxygen::HTTPMethod::POST) {
            reply405MethodNotAllowed("Only POST method is allowed.");
            return;
        }

        auto contentType = reqHeaders->getHeaders().getSingleOrEmpty("Content-Type");

        // Trim leading whitespace to be more robust
        contentType.erase(0, contentType.find_first_not_of(" \t"));
        if (!contentType.starts_with("application/json")) {
            reply415UnsupportedMediaType("Content-Type must be application/json.");
            return;
        }

        auto path = reqHeaders->getPath();

        if (path != SETTLE_PATH && path != VERIFY_PATH) {
            reply400BadRequest("Invalid endpoint. Use settle or verify.");
            return;
        }

        path_ = path;


    } catch ( std::exception& e ) {
        spdlog::critical( "onRequestStart exception" );
        printNestedException( e );
        reply500InternalError( "Could not process x402 request start." );
    } catch (...) {
        spdlog::critical( "onRequestStart unknown exception" );
        reply500InternalError( "Could not process x402 request start." );
    };
}


void FacilitatorProcessor::replyToClientWithError( const HttpError& httpError ) {
    auto httpErrorMessage = httpError.message();
    switch ( httpError.type() ) {
    case ErrorType::ERR_BAD_REQUEST:
        reply400BadRequest( httpErrorMessage );
        break;
    case ErrorType::ERR_INTERNAL_SERVER_ERROR:
        reply500InternalError( httpErrorMessage );
        break;
    case ErrorType::ERR_BAD_GATEWAY:
        // cant happen for facilitator
        reply500InternalError( httpErrorMessage );
        break;
    default:
        // cant happen
        CHECK_STATE( false );
    }
}


bool FacilitatorProcessor::isReplySent() const {
    return state_ == FacilitatorProcessorState::ERROR_SENT ||
           state_ == FacilitatorProcessorState::SUCCESS_REPLY_SENT;
}

void FacilitatorProcessor::onRequestFullyReceived(
    const std::unique_ptr< proxygen::HTTPMessage >& , const string& body ) noexcept {
    try {
        if (isReplySent())
            return;

        nlohmann::json jsonBody;

        try {
            jsonBody = nlohmann::json::parse( body );
        } catch ( nlohmann::json::parse_error& e ) {
            reply400BadRequest( "Invalid JSON in request body." );
            return;
        }


        nlohmann::json result;

        if (path_ == SETTLE_PATH)
            result = app_.easyNetFacilitator()->processSettleRequest( jsonBody );
        else  {
            CHECK_STATE( path_ == VERIFY_PATH );
            result = app_.easyNetFacilitator()->processVerifyRequest( jsonBody );
        }


        std::string responseBody = result.dump(); // assuming result is nlohmann::json

        sendResponse({200, "OK"}, STANDARD_HEADERS, responseBody);
        state_ = FacilitatorProcessorState::SUCCESS_REPLY_SENT;

    } catch ( std::exception& e ) {
        spdlog::critical( "onRequestFullyReceived exception" );
        printNestedException( e );
        reply500InternalError( "Could not process settlement request." );
    } catch (...) {
        spdlog::critical( "onRequestFullyReceived unknown exception" );
        reply500InternalError( "Could not process settlement request." );
    }
}

void FacilitatorProcessor::onBodySizeIncrease( size_t newSize ) {
    if (isReplySent())
        return;
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024;
    if ( newSize > MAX_BODY_SIZE ) {
        reply413PayloadTooLarge(
            "Request body too large. Maximum allowed is 1MByte. You can increase this limit in "
            "machinepay config if needed." );
    }
}

const std::vector< std::pair< std::string, std::string > > FacilitatorProcessor::STANDARD_HEADERS = {
    { "Content-Type", "application/json" }
};