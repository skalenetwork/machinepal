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

// --- Helper Functions ---

// ---------- tiny utils ----------
std::optional<std::string> MachinePayConfigLoader::getenvOpt(const char *key) {
    if (const char *v = std::getenv(key)) return std::string(v);
    return std::nullopt;
}

std::string MachinePayConfigLoader::readSecretFileFirstLine(const std::string &path,
                                                 const std::string &fallback) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return fallback;
        std::string line;
        std::getline(f, line);
        return line;
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::readSecretFileFirstLine failed: ", ex);
    }
}


static inline bool yamlTagIs(const YAML::Node& node, std::string_view shortTag /* like "!!bool" */) {
    const std::string& tag = node.Tag(); // may be "", "!!str", or canonical "tag:yaml.org,2002:str"
    if (tag.empty()) return false;
    if (tag == shortTag) return true;
    // Map "!!foo" -> "tag:yaml.org,2002:foo"
    if (shortTag.rfind("!!", 0) == 0) {
        std::string canon = "tag:yaml.org,2002:" + std::string(shortTag.substr(2));
        return tag == canon;
    }
    return false;
}


static json yamlNodeToJson(const YAML::Node& node) {
    using Type = YAML::NodeType::value;

    switch (node.Type()) {
        case Type::Null:
            return nullptr;

        case Type::Scalar: {
            const std::string s = node.Scalar();

            // Explicit tags first
            if (yamlTagIs(node, "!!bool")) {
                return node.as<bool>();
            }
            if (yamlTagIs(node, "!!int")) {
                try { return node.as<int64_t>(); }
                catch (const YAML::BadConversion&) {}
                try { return node.as<uint64_t>(); }
                catch (const YAML::BadConversion&) {}
                return s;
            }
            if (yamlTagIs(node, "!!float")) {
                try {
                    double d = node.as<double>();
                    if (std::isfinite(d)) return d;
                    return s;
                } catch (const YAML::BadConversion&) { return s; }
            }
            if (yamlTagIs(node, "!!str")) {
                return s;
            }

            // --- No tag: try implicit types ---
            // Try bool first
            try {
                return node.as<bool>();
            } catch (const YAML::BadConversion&) {}

            // Try int64
            try {
                return node.as<int64_t>();
            } catch (const YAML::BadConversion&) {}

            // Try uint64
            try {
                return node.as<uint64_t>();
            } catch (const YAML::BadConversion&) {}

            // Try double
            try {
                double d = node.as<double>();
                if (std::isfinite(d)) return d;
            } catch (const YAML::BadConversion&) {}

            // Fallback: string
            return s;
        }

        case Type::Sequence: {
            json arr = json::array();
            for (const auto& it : node) {
                arr.push_back(yamlNodeToJson(it));
            }
            return arr;
        }

        case Type::Map: {
            json obj = json::object();
            for (auto it = node.begin(); it != node.end(); ++it) {
                std::string keyStr;
                if (it->first.Type() == Type::Scalar &&
                    (it->first.Tag().empty() || yamlTagIs(it->first, "!!str"))) {
                    keyStr = it->first.Scalar();
                } else {
                    keyStr = YAML::Dump(it->first);
                }
                obj[keyStr] = yamlNodeToJson(it->second);
            }
            return obj;
        }

        case Type::Undefined:
        default:
            return nullptr;
    }
}

json MachinePayConfigLoader::yamlToJson(const std::string &yaml_path) {
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        return yamlNodeToJson(root);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Error loading YAML file '" << yaml_path << "': " << ex.what();
        std::throw_with_nested(
            std::runtime_error(std::string("Error loading YAML file '") + yaml_path + "': " + ex.what()));
    }
}


bool MachinePayConfigLoader::asBool(const std::string &s) {
    return s == "1" || s == "true" || s == "TRUE" || s == "yes" || s == "on";
};


/**
 * @brief Applies an environment variable as a string to a JSON object at a given path.
 */
void MachinePayConfigLoader::applyStringEnv(json &j, const json::json_pointer &path, const char *envVar) {
    try {
        if (auto v = getenvOpt(envVar)) {
            j[path] = *v;
        }
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::applyStringEnv failed: ", ex);
    }
}

/**
 * @brief Applies an environment variable as a boolean to a JSON object at a given path.
 */
void MachinePayConfigLoader::applyBoolEnv(json &j, const json::json_pointer &path, const char *envVar) {
    try {
        if (auto v = getenvOpt(envVar)) {
            j[path] = asBool(*v);
        }
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::applyBoolEnv failed: ", ex);
    }
}

/**
 * @brief Applies an environment variable as an integer to a JSON object at a given path.
 */
void MachinePayConfigLoader::applyIntEnv(json &j, const json::json_pointer &path, const char *envVar) {
    try {
        if (auto v = getenvOpt(envVar)) {
            j[path] = std::stoi(*v);
        }
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::applyIntEnv failed: ", ex);
    }
}


// --- Refactored applyEnvOverrides Function ---

void MachinePayConfigLoader::applyEnvOverrides(json &j) {
    try {
        // ---------- server ----------
        applyBoolEnv(j, "/server/enable_http"_json_pointer, "SERVER_ENABLE_HTTP");
        applyBoolEnv(j, "/server/enable_https"_json_pointer, "SERVER_ENABLE_HTTPS");
        applyIntEnv(j, "/server/http_listen_port"_json_pointer, "SERVER_HTTP_PORT");
        applyIntEnv(j, "/server/https_listen_port"_json_pointer, "SERVER_HTTPS_PORT");

        // ---------- server.tls ----------
        applyStringEnv(j, "/server/tls/cert_file"_json_pointer, "SERVER_TLS_CERT_FILE");
        applyStringEnv(j, "/server/tls/key_file"_json_pointer, "SERVER_TLS_KEY_FILE");
        applyStringEnv(j, "/server/tls/key_pass_file"_json_pointer, "SERVER_TLS_KEY_PASS_FILE");
        applyStringEnv(j, "/server/tls/ca_file"_json_pointer, "SERVER_TLS_CA_FILE");

        // ---------- facilitator ----------
        applyStringEnv(j, "/facilitator/type"_json_pointer, "FACILITATOR_TYPE");
        applyStringEnv(j, "/facilitator/base_url"_json_pointer, "FACILITATOR_BASE_URL");
        applyStringEnv(j, "/facilitator/api_key_file"_json_pointer, "FACILITATOR_API_KEY_FILE");
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::applyEnvOverrides failed: ", ex);
    }
}


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

// ---------- JSON Schema validation ----------
struct SchemaValidationErrorHandler : public nlohmann::json_schema::error_handler {

    std::string errorMessage;

    void error(const nlohmann::json_pointer<std::string>& ptr, const json& instance, const std::string& message) override {
        std::ostringstream oss;
        oss << "ERROR IN CONFIG FILE at path: " << ptr.to_string() << "\n"
            << "  Value: " << instance.dump(2) << "\n"
            << "  Instance type: " << instance.type_name() << "\n"
            << "  Error:    " << message << "\n";
        errorMessage = oss.str();
        throw std::runtime_error(errorMessage);
    }
};

void MachinePayConfigLoader::validateJson(const json &j) {
    json schema;
    try {
        schema = json::parse(MachinePayConfigSchemaJson);
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::validateJson failed to parse schema: ", ex);
    }

    SchemaValidationErrorHandler errHandler;
    try {
        json_validator validator;
        validator.set_root_schema(schema); // throws on invalid schema

        validator.validate(j, errHandler); // throws on validation error
    } catch (const std::exception &ex) {
        std::string errorMsg = std::string("MachinePayConfigLoader::validateJson Invalid config file : "
                                           "failed to validate config against schema:\n") +
                                               errHandler.errorMessage;
        LOG_AND_RETHROW_NESTED(errorMsg, ex);
    }
}


// ---------- Orchestrator ----------
void MachinePayConfigLoader::load(const std::string &yaml_path,
                                    const std::string &schema_path) {
    try {
        json j = yamlToJson(yaml_path);
        applyEnvOverrides(j);
        resolveSecrets(j);
        validateJson(j);
        toMachinePayConfig(j);
    } catch (const std::exception &ex) {
        LOG_AND_RETHROW_NESTED("MachinePayConfigLoader::load failed: ", ex);
    }
}

void MachinePayConfigLoader:: toMachinePayConfig(const nlohmann::json &j) {
    // serverConfig
    const auto &js = j.at("server");
    const auto &jt = js.at("tls");
    TlsConfig tlsConfig(
        MachinePayConfigLoader::getStringWithDefault(jt, "cert_file", ""),
        MachinePayConfigLoader::getStringWithDefault(jt, "key_file", ""),
        MachinePayConfigLoader::getStringWithDefault(jt, "key_pass_file", ""),
        (jt.contains("ca_file") && !jt.at("ca_file").is_null()) ? std::optional<std::string>(jt.at("ca_file").get<std::string>()) : std::nullopt
    );
    ServerConfig serverConfig(
        js.at("enable_http").get<bool>(),
        js.at("enable_https").get<bool>(),
        js.at("http_listen_port").get<uint16_t>(),
        js.at("https_listen_port").get<uint16_t>(),
        MachinePayConfigLoader::getStringWithDefault(js, "bind_ip", "0.0.0.0"),
        tlsConfig
    );

    // FacilitatorConfig
    const auto &jfaci = j.at("facilitator");
    FacilitatorConfig facilitatorConfig(
        MachinePayConfigLoader::getStringWithDefault(jfaci, "type", ""),
        MachinePayConfigLoader::getStringWithDefault(jfaci, "base_url", ""),
        (jfaci.contains("api_key_file") && !jfaci.at("api_key_file").is_null()) ? std::optional<std::string>(jfaci.at("api_key_file").get<std::string>()) : std::nullopt
    );

    latestConfig_ = std::make_shared<MachinePayConfig>(serverConfig, facilitatorConfig);
}

// Helper to get a string from a json object with a default value
std::string MachinePayConfigLoader::getStringWithDefault(const nlohmann::json& j, const std::string& key, const std::string& defaultValue) {
    if (j.contains(key) && !j.at(key).is_null()) {
        return j.at(key).get<std::string>();
    }
    return defaultValue;
}

std::shared_ptr<MachinePayConfig> MachinePayConfigLoader::latestConfig() {
    CHECK_STATE(latestConfig_);
    return latestConfig_;
}

// Definition of the static member
std::shared_ptr<MachinePayConfig> MachinePayConfigLoader::latestConfig_ = nullptr;

