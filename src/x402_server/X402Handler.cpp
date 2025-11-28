#include "X402Handler.h"
#include "MachinePayCommon.h"

#include "ProxygenResponseSender.h"
#include "examples/PaymentExamples.h"
#include "x402_protocol/HttpEndpointConnection.h"
#include "x402_protocol/IProcessor.h"

using namespace proxygen;


X402Handler::X402Handler( MachinePayApp& app ) : app_( app ) {
    // we take the latest condig at the start
    config_ = app_.configManager()->latestConfig();
    CHECK_STATE( config_ );
}


void X402Handler::onRequest( std::unique_ptr< HTTPMessage > _headers ) noexcept {
    if ( internalErrorSent_ )
        return;

    try {
        CHECK_STATE( self_ );

        responseSender_ = ProxygenResponseSender::makeShared(downstream_ ,
            folly::EventBaseManager::get()->getEventBase() );
        reqHeaders_ = std::move( _headers );
        auto weakResponseSender = std::weak_ptr< IResponseSender >( responseSender_ );
        if ( reqHeaders_->getPath().starts_with( EASYNET_FACILITATOR_PREFIX ) ) {
            processor_ = app_.makeFacilitatorProcessor( weakResponseSender );
        } else {
            processor_ = app_.makeX402Processor( weakResponseSender );
        }

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


void X402Handler::onError( proxygen::ProxygenError _err ) noexcept  {
    spdlog::error( "X402Handler::onError called: {}", proxygen::getErrorString( ( _err ) ) );
    responseSender().reset();
    // clean object if not used by different thread
    self_.reset();
}



void X402Handler::requestComplete() noexcept {
    // clean object if not used by different thread
    self_.reset();
}

void X402Handler::onUpgrade( proxygen::UpgradeProtocol /*_prot*/ ) noexcept {
    // No upgrade handling needed for now
}