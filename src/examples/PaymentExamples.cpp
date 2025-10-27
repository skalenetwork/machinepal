#include "MachinePayCommon.h"
#include "PaymentExamples.h"


const string PaymentExamples::EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA = R"({
    "scheme": "exact",
    "network": "base-sepolia",
    "maxAmountRequired": "12000000000000000000",
    "resource": "https://jsonplaceholder.typicode.com/posts/1",
    "description": "",
    "mimeType": "application/json",
    "outputSchema": null,
    "payTo": "0x2222222222222222222222222222222222222222",
    "maxTimeoutSeconds": 600,
    "asset": "0x036CbD53842c5426634e7929541eC2318f3dCF7e",
    "extra": {
        "name": "USDC",
        "version": "2"
    }
})";


const string PaymentExamples::EXACT_UCDC_PAYMENT_PAYLOAD_CB_SEPOLIA = R"({
    "x402Version": 1,
    "scheme": "exact",
    "network": "base-sepolia",
    "payload": {
        "signature": "0x9f3b3e8c8b1a68d08337a42b1b1e6d0562a15c0e4b4c30d2c6a9f965f4a2f53256e0d4f8a5c20dcb9b1d3b3c2f3e87cbf5e99c2b2f07e0b8c2a6f61d2f8f4c6a1b",
        "authorization": {
            "from": "0x1111111111111111111111111111111111111111",
            "to": "0x2222222222222222222222222222222222222222",
            "value": "1000",
            "validAfter": "1716150000",
            "validBefore": "1716153600",
            "nonce": "0x1234567890abcdef"
        }
    }
})";
