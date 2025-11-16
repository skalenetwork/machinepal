#include "X402Handler.h"
#include "MachinePayCommon.h"

#include "ProxygenResponseSender.h"
#include "examples/PaymentExamples.h"
#include "x402_protocol/BackendConnection.h"

using namespace proxygen;


void X402Handler::onRequest( std::unique_ptr< HTTPMessage > _headers ) noexcept {
    std::shared_ptr< IResponseSender > responseSender =
        std::make_shared< ProxygenResponseSender >( downstream_ );
    try {
        CHECK_STATE( self_ );
        reqHeaders_ = std::move( _headers );
        if ( reqHeaders_->getPath().starts_with( "/machinepay-api-easynet/" ) ) {
            processor_ = app_.makeEasyNetProcessor( responseSender );
        } else {
            processor_ = app_.makeX402Processor( responseSender );
        }

        processor_->onRequestStart( reqHeaders_ );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onRequest: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown error in onRequest" );
        sendInternalError();
    }
}

void X402Handler::onBody( std::unique_ptr< folly::IOBuf > _body ) noexcept {
    try {
        CHECK_STATE( self_ );
        if ( !_body )
            return;
        _body->coalesce();
        bodyBuffer_.append( reinterpret_cast< const char* >( _body->data() ), _body->length() );
        processor_->onBodySizeIncrease( bodyBuffer_.size() );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onBody: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown error in onBody" );
        sendInternalError();
    }
}


void X402Handler::sendInternalError() {
    std::shared_ptr< IResponseSender > responseSender =
        std::make_shared< ProxygenResponseSender >( downstream_ );
    responseSender->sendResponse(
        { 500, "Server Error" }, { { "Content-Type", "text/plain" } }, "Internal server error." );
}
void X402Handler::onEOM() noexcept {
    try {
        CHECK_STATE( self_ );
        if ( !processor_ ) {
            spdlog::critical( "X402Handler::onEOM() called without processor_" );
            return;
        }
        processor_->onRequestFullyReceived( reqHeaders_, bodyBuffer_ );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onEOM: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown errror in onEOM" );
        sendInternalError();
    }
}