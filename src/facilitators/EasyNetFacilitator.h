#pragma once


class EasyNetDb;
class PaymentRequirements;
class PaymentPayload;
class MachinePayApp;
class EasyNetFacilitator {
public:
    explicit EasyNetFacilitator(MachinePayApp& app);
    nlohmann::json processSettleRequest( const nlohmann::json& settlementRequestJson);
    pair< ptr< PaymentPayload >, ptr< PaymentRequirements > > processVerifyRequestUnsafe(
        const nlohmann::json& verifyRequestJson, optional< string >& error, EasyNetDb& db ) const;
    nlohmann::json processVerifyRequest( const nlohmann::json& verifyRequestJson);

private:
    MachinePayApp& app_;
    u256 chainId_;
    mutable std::shared_mutex mutex_;
};
