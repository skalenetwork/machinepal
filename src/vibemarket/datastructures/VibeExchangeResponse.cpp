#include "MachinePalCommon.h"

#include "VibeExchangeResponse.h"

#include "config/JsonUtils.h"

VibeExchangeResponse VibeExchangeResponse::fromJson(const nlohmann::json& j)
{
    // Parse using base implementation and return as derived.
    const McpResponse base = McpResponse::fromJson(j);

    VibeExchangeResponse out{};
    out.setIsError(base.isError());
    out.setTextContent(base.textContent());
    out.setJsonContent(base.jsonContent());
    return out;
}
