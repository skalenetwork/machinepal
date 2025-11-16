#pragma once
#include "../payment/datastructures/SettlementRequest.h"
#include "../payment/datastructures/SettlementResponse.h"
#include "MachinePayApp.h"
#include "config/subconfigs/FacilitatorConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "db/MachinePayDb.h"
#include <memory>
#include <variant>

using std::variant;
using std::shared_ptr;

class FacilitatorClientManager {
public:
    explicit FacilitatorClientManager(MachinePayApp& app);
    variant< SettlementResponse, HttpError > routeToFacilitatorAndSettle(
        const NetworkConfig& networkConfig, SettlementRequest& settlementRequest );



private:
    MachinePayApp& app_;
};

