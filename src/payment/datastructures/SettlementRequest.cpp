#include "MachinePayCommon.h"
#include "SettlementRequest.h"
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
    j["paymentPayload"] = paymentPayload()->toJson();
    j["paymentRequirements"] = paymentRequirements()->toJson();
    return j;
}

SettlementRequest SettlementRequest::fromJson(const nlohmann::json& j) {
    ptr<PaymentPayload> payload;
    ptr<PaymentRequirements> requirements;
    if (j.contains("paymentPayload") && !j["paymentPayload"].is_null()) {
        payload = PaymentPayload::fromJson(j["paymentPayload"]);
    }
    if (j.contains("paymentRequirements") && !j["paymentRequirements"].is_null()) {
        requirements = PaymentRequirements::fromJson(j["paymentRequirements"]);
    }
    return SettlementRequest(payload, requirements);
}
