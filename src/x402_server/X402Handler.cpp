#include "X402Handler.h"
#include "MachinePayCommon.h"

#include "ProxygenResponseSender.h"
#include "examples/PaymentExamples.h"
#include "x402_protocol/BackendConnection.h"
#include "x402_protocol/IProcessor.h"

using namespace proxygen;


void X402Handler::onRequest( std::unique_ptr< HTTPMessage > _headers ) noexcept {
    if ( internalErrorSent_ )
        return;

    try {
        CHECK_STATE( self_ );
        responseSender_ = std::make_shared< ProxygenResponseSender >( downstream_ ,
            folly::EventBaseManager::get()->getEventBase() );
        reqHeaders_ = std::move( _headers );
        if ( reqHeaders_->getPath().starts_with( EASYNET_FACILITATOR_PREFIX ) ) {
            processor_ = app_.makeFacilitatorProcessor( responseSender_ );
        } else {
            processor_ = app_.makeX402Processor( responseSender_ );
        }

        // processor_ is now of type std::shared_ptr<IProcessorInterface>
        processor()->onRequestStart( reqHeaders_ );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onRequest: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown error in onRequest" );
        sendInternalError();
    }
}

void X402Handler::onBody( std::unique_ptr< folly::IOBuf > _body ) noexcept {
    if ( internalErrorSent_ )
        return;

    try {
        CHECK_STATE( self_ );
        if ( !_body )
            return;
        if ( processor()->isReplySent() ) {
            return;
        }

        _body->coalesce();
        bodyBuffer_.append( reinterpret_cast< const char* >( _body->data() ), _body->length() );
        processor()->onBodySizeIncrease( bodyBuffer_.size() );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onBody: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown error in onBody" );
        sendInternalError();
    }
}


void X402Handler::sendInternalError() {
    internalErrorSent_ = true;
    if ( !responseSender_ ) {
        spdlog::critical( "Response sender not available in sendInternalError" );
        return;
    }
    responseSender_->sendResponse(
        { 500, "Server Error" }, { { "Content-Type", "text/plain" } },
        "Internal server error." );
}
void X402Handler::onEOM() noexcept {
    if ( internalErrorSent_ )
        return;

    try {
        CHECK_STATE( self_ );
        if ( processor()->isReplySent() ) {
            return;
        }
        processor()->onRequestFullyReceived( reqHeaders_, bodyBuffer_ );
    } catch ( const std::exception& e ) {
        spdlog::critical( "Error in onEOM: {}", e.what() );
        sendInternalError();
    } catch ( ... ) {
        spdlog::critical( "Unknown errror in onEOM" );
        sendInternalError();
    }
}