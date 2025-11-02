#include "SettlementRequest.h"

#include <nlohmann/json.hpp>
#include "MachinePayCommon.h" // added for CHECK_STATE macro
#include "PaymentPayload.h"
#include "PaymentRequirements.h"



ptr<PaymentPayload> SettlementRequest::paymentPayload() const {
    CHECK_STATE(paymentPayload_);
    return paymentPayload_;
}

ptr<PaymentRequirements> SettlementRequest::paymentRequirements() const {
    CHECK_STATE(paymentRequirements_);
    return paymentRequirements_;
}

SettlementRequest::SettlementRequest( const ptr<PaymentPayload>& paymentPayload,
    const ptr<PaymentRequirements>& paymentRequirements )
    : paymentPayload_(paymentPayload), paymentRequirements_(paymentRequirements) {
    CHECK_STATE(paymentPayload_);
    CHECK_STATE(paymentRequirements_);

}

nlohmann::json SettlementRequest::toJson() const {
    nlohmann::json j;
    j["paymentPayload"] = paymentPayload_ ? paymentPayload_->toJson() : nullptr;
    j["paymentRequirements"] = paymentRequirements_ ? paymentRequirements_->toJson() : nullptr;
    return j;
}

SettlementRequest SettlementRequest::fromJson(const nlohmann::json& j) {
    ptr<PaymentPayload> payload;
    ptr<PaymentRequirements> requirements;
    if (j.contains("paymentPayload") && !j["paymentPayload"].is_null()) {
        payload = std::make_shared<PaymentPayload>(PaymentPayload::fromJson(j["paymentPayload"]));
    }
    if (j.contains("paymentRequirements") && !j["paymentRequirements"].is_null()) {
        requirements = std::make_shared<PaymentRequirements>(PaymentRequirements::fromJson(j["paymentRequirements"]));
    }
    return SettlementRequest(payload, requirements);
}
