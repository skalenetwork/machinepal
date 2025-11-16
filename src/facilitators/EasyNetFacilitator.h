#pragma once


class EasyNetDb;
class PaymentRequirements;
class PaymentPayload;
class MachinePayApp;
class EasyNetFacilitator {
public:
    explicit EasyNetFacilitator(MachinePayApp& app);
    nlohmann::json settleLocal( const nlohmann::json& settlementRequestJson);
    pair< ptr< PaymentPayload >, ptr< PaymentRequirements > > verifyUnsafe(
        const nlohmann::json& verifyRequestJson, optional< string >& error, EasyNetDb& db ) const;
    nlohmann::json verifyLocal( const nlohmann::json& verifyRequestJson, EasyNetDb& db );

private:
    MachinePayApp& app_;
    u256 chainId_;
    mutable std::shared_mutex mutex_;
};
