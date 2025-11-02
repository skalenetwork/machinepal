#pragma once

namespace x402
{
    enum class State
    {
        START,
        ERROR_SENT,
        SUCCESS_PAYMENT_REQUIRED_SENT,
        PAYMENT_HEADER_RECEIVED,
        SUCCESS_RESOURCE_SENT
    };
}