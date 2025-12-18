#pragma once


class EasyNetDb;
class PaymentRequirements;
class PaymentPayload;
class MachinePalApp;
class EasyNetFacilitator {
public:
    explicit EasyNetFacilitator(MachinePalApp& app);
    nlohmann::json processSettleRequest( const nlohmann::json& settlementRequestJson);
    pair< ptr< PaymentPayload >, ptr< PaymentRequirements > > processVerifyRequestUnsafe(
        const nlohmann::json& verifyRequestJson, optional< string >& error, EasyNetDb& db ) const;
    nlohmann::json processVerifyRequest( const nlohmann::json& verifyRequestJson);

private:
    MachinePalApp& app_;
    u256 chainId_;
    mutable std::shared_mutex mutex_;
};
