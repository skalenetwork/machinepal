#pragma once

#include <string>

#include "McpRequest.h"
#include "crypto/TokenAmount.h"


class VibeBuyRequest : McpRequest{
    std::string token;
    TokenAmount amount;


public:
    static VibeBuyRequest fromJson(const nlohmann::json& j);

    const std::string& getToken() const { return token; }
    const TokenAmount& getAmount() const { return amount; }
};
