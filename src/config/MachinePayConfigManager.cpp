#include "MachinePayConfigManager.h"

#include "common.h"
#include "MachinePayConfigLoader.h"
#include "MachinePayConfig.h"
#include "config/MachinePayConfigSchema.h"
#include "init/InitLibs.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <unordered_set>


using nlohmann::json;
using nlohmann::json_schema::json_validator;;
using namespace nlohmann::literals; // Enables the _json_pointer literal




// ---------- Resolve secret files to actual values ----------
void MachinePayConfigLoader::resolveSecrets(json &j) {
    try {
        if (j.contains("database") && j["database"].is_object()) {
            const std::string dbFile = j["database"].value("passwordFile", "");
            j["database"]["password"] = readSecretFileFirstLine(dbFile, /*fallback*/ "");
        }
        // Only resolve password in db, not jwt
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::resolveSecrets failed: ", ex);
    }
}




// ---------- Orchestrator ----------
void MachinePayConfigManager::loadConfig(const std::string &yaml_path) {
    latestConfig_ = MachinePayConfigLoader::loadConfig(yaml_path);
}


std::shared_ptr<MachinePayConfig> MachinePayConfigLoader::latestConfig() {
    CHECK_STATE(latestConfig_);
    return latestConfig_;
}

// Definition of the static member
std::shared_ptr<MachinePayConfig> MachinePayConfigLoader::latestConfig_ = nullptr;

