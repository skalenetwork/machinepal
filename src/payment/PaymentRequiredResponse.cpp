#include "MachinePayCommon.h"
#include "PaymentRequiredResponse.h"

using json = nlohmann::json;

PaymentRequiredResponse PaymentRequiredResponse::fromJson(const json &j) {
    PaymentRequiredResponse out;
    if (j.contains("x402Version") && j["x402Version"].is_number_integer()) {
        out = PaymentRequiredResponse();
        out = PaymentRequiredResponse(j["x402Version"].get<int>(), {}, std::nullopt);
    }
    // accepts (array of PaymentRequirements)
    if (j.contains("accepts") && j["accepts"].is_array()) {
        std::vector<PaymentRequirements> vec;
        for (const auto &elem : j["accepts"]) {
            // PaymentRequirements::fromJson returns shared_ptr
            auto pr = PaymentRequirements::fromJson(elem);
            if (pr) vec.push_back(*pr);
        }
        if (out.x402Version() == 0) {
            // construct with default version 1 if not set earlier
            out = PaymentRequiredResponse(1, std::move(vec), std::nullopt);
        } else {
            out = PaymentRequiredResponse(out.x402Version(), std::move(vec), std::nullopt);
        }
    }
    // error
    if (j.contains("error") && !j["error"].is_null()) {
        auto err = j["error"].get<std::string>();
        if (out.accepts().empty() && out.x402Version() == 0) {
            out = PaymentRequiredResponse(1, {}, err);
        } else {
            out = PaymentRequiredResponse(out.x402Version() == 0 ? 1 : out.x402Version(), out.accepts(), err);
        }
    }
    // If nothing was parsed, ensure defaults
    if (out.x402Version() == 0 && out.accepts().empty() && !out.error().has_value()) {
        return PaymentRequiredResponse(1, {}, std::nullopt);
    }
    return out;
}

json PaymentRequiredResponse::toJson() const {
    json j;
    j["x402Version"] = x402Version_;
    j["accepts"] = json::array();
    for (const auto &p : accepts_) {
        j["accepts"].push_back(p.toJson());
    }

    j["error"] = error_;

    return j;

}