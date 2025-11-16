#pragma once
#include <proxygen/lib/http/HTTPMethod.h>

#include "FacilitatorProcessorState.h"
#include "HttpError.h"
#include "IProcessor.h"
#include "IResponseSender.h"


class SettlementResponse;
class ResourceConfig;

namespace proxygen {
enum class HTTPMethod;
class ResponseHandler;
class HTTPMessage;
}  // namespace proxygen

class MachinePayApp;  // Forward declaration
class MachinePayConfig;
class OrganizationConfig;

class FacilitatorProcessor : public IProcessor {
public:

    explicit FacilitatorProcessor( MachinePayApp& app, ptr< IResponseSender >& responseSender );
    void reply400BadRequest( const std::string& message );

    void onRequestStart( const std::unique_ptr< proxygen::HTTPMessage >& headers ) noexcept override;
    bool settle( std::string& responseBody );

    void replyToClientWithError( const HttpError& httpError ) override;
    bool isReplySent() const override;

    void onRequestFullyReceived(
        const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders, const string& body ) noexcept override;
    void onBodySizeIncrease( size_t newSize ) override;
    static const std::vector< std::pair< std::string, std::string > > STANDARD_HEADERS;

private:

    void sendResponse( const std::pair< uint16_t, std::string >& statusAndMessage,
        const std::vector< std::pair< std::string, std::string > >& headers,
        const std::string& body );

    string getErrorBody( const std::string& message );

    void reply500InternalError( const std::string& message );

    void reply405MethodNotAllowed( const std::string& message );
    void reply413PayloadTooLarge( const std::string& message );

    void reply415UnsupportedMediaType( const std::string& message );


    [[nodiscard]] ptr< MachinePayConfig > config() const {
        CHECK_STATE( config_ );
        return config_;
    }


    MachinePayApp& app_;
    ptr< MachinePayConfig > config_;
    std::string decodedPath_;
    ptr< IResponseSender > responseSender_;
    FacilitatorProcessorState state_ = FacilitatorProcessorState::START;
    // initially set to non-supported value
    proxygen::HTTPMethod method_ = proxygen::HTTPMethod::TRACE;
};
