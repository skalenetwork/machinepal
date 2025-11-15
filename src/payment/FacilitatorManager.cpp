#include "FacilitatorManager.h"
#include "MachinePayApp.h"
#include "db/MachinePayDb.h"
#include "spdlog/spdlog.h"

FacilitatorManager::FacilitatorManager(MachinePayApp& app) : app_(app) {}

variant<SettlementResponse, HttpError> FacilitatorManager::routeToFacilitator(
    const NetworkConfig& networkConfig,
    SettlementRequest& settlementRequest) {
    try {
        auto facilitator = networkConfig.facilitator();
        CHECK_STATE(facilitator);
        auto paymentPayload = settlementRequest.paymentPayload();
        auto result = facilitator.value()->settlePayment(paymentPayload);
        if (holds_alternative<HttpError>(result)) {
            return std::get<HttpError>(result);
        }
        return std::get<SettlementResponse>(result);
    } catch (const std::exception& e) {
        spdlog::error("Error in routeToFacilitator: {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("Facilitator error: ") + e.what());
    }
}

void FacilitatorManager::recordFacilitatorAction(const SettlementRequest& request,
    const SettlementResponse& response) {
    auto db = app_.machinePayDB();
    // Example: Log or store facilitator action. Extend as needed.
    spdlog::info("Facilitator action recorded for payment: {}", request.paymentPayload()->payload()->authorization()->from().toHex(PREFIX_0x));
    // db->saveFacilitatorAction(request, response); // Implement as needed
}

