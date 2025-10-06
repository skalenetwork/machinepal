#pragma once
#include <string>
#include <glog/types.h>
#include <optional>
#include <cstdint>

struct TlsConfig {
    std::string certFile;             // required
    std::string keyFile;              // required
    std::string keyPassFile;          // required (but secret file path)
    std::optional<std::string> caFile; // optional (system CA used if not set)
};

struct FacilitatorConfig {
    std::string type;                 // e.g., "cdp" or "x402"
    std::string baseUrl;              // required
    std::optional<std::string> apiKeyFile; // optional (secret file path)
};

struct ServerConfig {
    bool httpEnabled; // required
    bool httpsEnabled; // required
    uint16_t httpPort;
    uint16_t httpsPort;
    std::string bindIp; // default to INADDR_ANY
    TlsConfig tls;                    // nested struct for clarity
};

struct MachinePayConfig {
    ServerConfig server;
    FacilitatorConfig facilitator;
};