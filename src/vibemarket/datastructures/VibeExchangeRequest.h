#pragma once

#include <string>

#include "McpRequest.h"
#include "crypto/TokenAmount.h"


class VibeExchangeRequest : McpRequest{
    std::string inputToken;
    std::string outputToken;
    TokenAmount inputAmount;
    TokenAmount minOutputAmount;

public:
    static VibeExchangeRequest fromJson(const nlohmann::json& j);

    const std::string& getInputToken() const { return inputToken; }
    const std::string& getOutputToken() const { return outputToken; }
    const TokenAmount& getInputAmount() const { return inputAmount; }
    const TokenAmount& getMinOutputAmount() const { return minOutputAmount; }
};
