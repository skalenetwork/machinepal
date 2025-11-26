#pragma once
#include <proxygen/lib/http/HTTPMethod.h>
#include <optional>

#include "IBackendError.h"
#include "HttpError.h"
#include "IProcessor.h"
#include "IResponseSender.h"
#include "X402ProcessorState.h"
#include "payment/datastructures/Authorization.h"


class IBackendError;
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

class X402Processor : public IProcessor {
public:

    explicit X402Processor( MachinePayApp& app, ptr< IResponseSender >& responseSender );

    bool isReplySent() const override;

    bool reply402IfNoPaymentHeader( const std::unique_ptr< proxygen::HTTPMessage >& req );

    void onRequestStart( const std::unique_ptr< proxygen::HTTPMessage >& headers ) noexcept override;


    void replyToClientWithError( const HttpError& httpError ) override;
    void sendSettlementErrorResponse(
        ptr< Authorization > authorization, add_pointer_t< HttpError > error );

    void doPassThrough(const std::unique_ptr< proxygen::HTTPMessage >& requestHeaders,const string& requestBody);

    void onRequestFullyReceived(
        const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders, const string& body ) noexcept override;
    void onBodySizeIncrease( size_t newSize ) override;
    static const std::vector< std::pair< std::string, std::string > > STANDARD_HEADERS;

private:
    void reply402PaymentRequired( std::optional< SettlementResponse > errorResponse );
    void sendResponse( const std::pair< uint16_t, std::string >& statusAndMessage,
        const std::vector< std::pair< std::string, std::string > >& headers,
        const std::string& body );

    void reply400InvalidPayment( const std::string& message );

    void reply400ResourceNotFound(const std::string &message);

    string getErrorBody( const std::string& message );

    void reply500InternalError( const std::string& message );

    void reply502BadGateway( const std::string& message );

    void replyGenericHttpError(IBackendError &error);

    void replyPassThroughError( IBackendError& error );

    bool validateAndExtractSubDomainName(
        const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders );

    bool validateAndDecodePath( const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders );

    bool matchOrganization();

    bool validateMethod( const std::unique_ptr< proxygen::HTTPMessage >& reqHeaders );

    void reply200Success( const std::string& settlementInfo, std::string& proxyBody );

    [[nodiscard]] ptr< MachinePayConfig > config() const {
        CHECK_STATE( config_ );
        return config_;
    }

    [[nodiscard]] ptr< ResourceConfig > resource() const {
        CHECK_STATE( resource_ );
        return resource_;
    }


    [[nodiscard]] ptr< OrganizationConfig > organization() const {
        CHECK_STATE( organization_ );
        return organization_;
    }


    MachinePayApp& app_;
    ptr< MachinePayConfig > config_;
    std::string decodedPath_;
    std::string subDomainName_;
    ptr< OrganizationConfig > organization_;
    ptr< ResourceConfig > resource_;
    ptr< IResponseSender > responseSender_;
    X402ProcessorState state_ = X402ProcessorState::START;
    // initially set to non-supported value
    proxygen::HTTPMethod method_ = proxygen::HTTPMethod::TRACE;
};
