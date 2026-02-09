#include "MachinePalCommon.h"

#include "VibeExchangeRequest.h"

#include "config/JsonUtils.h"


VibeExchangeRequest VibeExchangeRequest::fromJson(const nlohmann::json& j)
{
    CHECK_STATE_JSON(j.is_object(), "VibeExchangeRequest::fromJson: expected JSON object", j);

    auto requireString = [&](const char* key) -> std::string {
        CHECK_STATE_JSON(j.contains(key),
            std::string("VibeExchangeRequest::fromJson: missing field: ") + key,
            j);
        CHECK_STATE_JSON(j.at(key).is_string(),
            std::string("VibeExchangeRequest::fromJson: field must be string: ") + key,
            j);
        return j.at(key).get<std::string>();
    };

    VibeExchangeRequest req;
    req.inputToken       = requireString("inputToken");
    req.outputToken      = requireString("outputToken");
    req.inputAmount      = TokenAmount::fromHexOrDecimal(requireString("inputAmount"));
    req.minOutputAmount  = TokenAmount::fromHexOrDecimal(requireString("minOutputAmount"));
    return req;
}
