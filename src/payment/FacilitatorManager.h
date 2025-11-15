#pragma once
#include "MachinePayApp.h"
#include "config/subconfigs/FacilitatorConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "datastructures/SettlementRequest.h"
#include "datastructures/SettlementResponse.h"
#include "db/MachinePayDb.h"
#include <variant>
#include <memory>

using std::variant;
using std::shared_ptr;

class FacilitatorManager {
public:
    explicit FacilitatorManager(MachinePayApp& app);

    variant<SettlementResponse, HttpError> routeToFacilitator(
        const NetworkConfig& networkConfig,
        SettlementRequest& settlementRequest);

    void recordFacilitatorAction(const SettlementRequest& request,
        const SettlementResponse& response);

private:
    MachinePayApp& app_;
};

