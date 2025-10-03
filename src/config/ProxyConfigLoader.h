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

    static bool asBool(const std::string& s);

private:
    static void applyEnvOverrides(nlohmann::json& j);
    static void resolveSecrets(nlohmann::json& j);
    static void validateJson(const nlohmann::json& j,
                             const std::string& schema_path);
    static ProxyConfig toProxyConfig(const nlohmann::json& j);

    // Utility helpers
    static std::optional<std::string> getenvOpt(const char* key);

    static void applyStringEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static void applyBoolEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static void applyIntEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static std::string readSecretFileFirstLine(const std::string& path,
                                         const std::string& fallback = "");
};