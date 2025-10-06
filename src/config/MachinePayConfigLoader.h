#pragma once


#include "MachinePayConfig.h"
#include "nlohmann/json.hpp"
#include <mutex>

// Source-of-truth loader:
// 1) Load YAML from file.
// 2) Convert to JSON.
// 3) Overlay environment variables (selected keys).
// 4) Resolve secret *files* (read contents).
// 5) Validate against JSON Schema.
// 6) Materialize AppConfig.
class MachinePayConfigLoader {


public:

    static std::shared_ptr<MachinePayConfig> loadConfig(const std::string &yaml_path);

private:

    // Helper: convert YAML (yaml-cpp node) to nlohmann::json
    static nlohmann::json yamlToJson(const std::string& yaml_path);

    static bool asBool(const std::string& s);


    static std::string getStringWithDefault(
        const nlohmann::json &j, const std::string &key, const std::string &defaultValue);


    static void applyEnvOverrides(nlohmann::json& j);
    static void resolveSecrets(nlohmann::json& j);
    static void validateJson(const nlohmann::json& j);
    static std::shared_ptr<MachinePayConfig> toMachinePayConfig(const nlohmann::json& j);

    // Utility helpers
    static std::optional<std::string> getenvOpt(const char* key);

    static void applyStringEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static void applyBoolEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static void applyIntEnv(nlohmann::json &j, const nlohmann::json_pointer<std::string> &path, const char *envVar);

    static std::string readSecretFileFirstLine(const std::string& path,
                                         const std::string& fallback = "");

    std::shared_ptr<MachinePayConfig> load(const std::string& yaml_path);


};