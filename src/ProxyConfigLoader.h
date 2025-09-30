#pragma once


#include "ProxyConfig.h"
#include "nlohmann/json.hpp"

// Source-of-truth loader:
// 1) Load YAML from file.
// 2) Convert to JSON.
// 3) Overlay environment variables (selected keys).
// 4) Resolve secret *files* (read contents).
// 5) Validate against JSON Schema.
// 6) Materialize AppConfig.
class ProxyConfigLoader {
public:
    // paths: YAML and JSON schema files
    static ProxyConfig load(const std::string& yaml_path,
                          const std::string& schema_path);

    // Helper: convert YAML (yaml-cpp node) to nlohmann::json
    static nlohmann::json yamlToJson(const std::string& yaml_path);

private:
    static void applyEnvOverrides(nlohmann::json& j);
    static void resolveSecrets(nlohmann::json& j);
    static void validateJson(const nlohmann::json& j,
                             const std::string& schema_path);
    static ProxyConfig toProxyConfig(const nlohmann::json& j);

    // Utility helpers
    static std::optional<std::string> getenvOpt(const char* key);
    static std::string readFileFirstLine(const std::string& path,
                                         const std::string& fallback = "");
};