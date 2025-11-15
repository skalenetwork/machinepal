#include "FacilitatorManager.h"
#include "MachinePayApp.h"
#include "db/MachinePayDb.h"
#include "spdlog/spdlog.h"

FacilitatorManager::FacilitatorManager(MachinePayApp& app) : app_(app) {}

variant< SettlementResponse, HttpError > FacilitatorManager::routeToFacilitatorAndSettle(
    const NetworkConfig& networkConfig, SettlementRequest& settlementRequest ) {
    if ( networkConfig.name() == "machinepay-easynet" ) {
        auto baseDb = app_.machinePayDB();
        auto db = std::dynamic_pointer_cast< EasyNetDb >( baseDb );
        CHECK_STATE( db );
        auto jsonResponse =
            networkConfig.facilitatorClient()->settleLocal( settlementRequest.toJson(), *db );
        return SettlementResponse::fromJsonString( jsonResponse.dump() );
    } else {
        auto facilitator = networkConfig.facilitator();
        CHECK_STATE( facilitator );
        auto paymentPayload = settlementRequest.paymentPayload();
        auto result = facilitator.value()->settlePayment( paymentPayload );
        if ( holds_alternative< HttpError >( result ) ) {
            return result;
        }
        return std::get< SettlementResponse >( result );
    }
}

