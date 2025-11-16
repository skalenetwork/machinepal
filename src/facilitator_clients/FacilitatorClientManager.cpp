#include "FacilitatorClientManager.h"
#include "MachinePayApp.h"
#include "db/MachinePayDb.h"
#include "spdlog/spdlog.h"

FacilitatorClientManager::FacilitatorClientManager(MachinePayApp& app) : app_(app) {}

variant< SettlementResponse, HttpError > FacilitatorClientManager::routeToFacilitatorAndSettle(
    const MachinePayConfig& machinePayConfig, SettlementRequest& settlementRequest ) {
    auto networkConfig = machinePayConfig.network();
    CHECK_STATE( networkConfig );

    auto facilitatorClient = machinePayConfig.facilitatorClient();
    CHECK_STATE( facilitatorClient );

    auto result = facilitatorClient->settle( settlementRequest.toJson());

    return SettlementResponse::fromJsonString( result.dump(  ) );
}

