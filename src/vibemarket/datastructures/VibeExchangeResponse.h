#pragma once


#include "McpResponse.h"


class VibeExchangeResponse : public McpResponse {
public:
    static VibeExchangeResponse fromJson(const nlohmann::json& j);
};
