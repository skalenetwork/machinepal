#pragma once

namespace x402 {
    enum class State {
        START,
        ERROR,
        PAYMENT_REQUIRED_SENT,
        PAYMENT_HEADER_RECEIVED,
        SUCCESS
    };
}

