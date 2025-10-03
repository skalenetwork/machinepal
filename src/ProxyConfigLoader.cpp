#include "common.h"
#include "ProxyConfigLoader.h"
#include "ProxyConfig.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <optional>


using nlohmann::json;
using nlohmann::json_schema::json_validator;;
using namespace nlohmann::literals; // Enables the _json_pointer literal

// --- Helper Functions ---

// ---------- tiny utils ----------
std::optional<std::string> ProxyConfigLoader::getenvOpt(const char *key) {
    if (const char *v = std::getenv(key)) return std::string(v);
    return std::nullopt;
}

std::string ProxyConfigLoader::readFileFirstLine(const std::string &path,
                                                 const std::string &fallback) {
    std::ifstream f(path);
    if (!f.is_open()) return fallback;
    std::string line;
    std::getline(f, line);
    return line;
}

// ---------- YAML -> JSON (recursive) ----------
static json yamlNodeToJson(const YAML::Node &node) {
    using Type = YAML::NodeType::value;
    switch (node.Type()) {
        case Type::Null: return nullptr;
        case Type::Scalar: return node.as<std::string>();
        case Type::Sequence: {
            json arr = json::array();
            for (auto &&it: node) arr.push_back(yamlNodeToJson(it));
            return arr;
        }
        case Type::Map: {
            json obj = json::object();
            for (auto it = node.begin(); it != node.end(); ++it) {
                obj[it->first.as<std::string>()] = yamlNodeToJson(it->second);
            }
            return obj;
        }
        default: return nullptr;
    }
}

json ProxyConfigLoader::yamlToJson(const std::string &yaml_path) {
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        return yamlNodeToJson(root);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Error loading YAML file '" << yaml_path << "': " << ex.what();
        std::throw_with_nested(
            std::runtime_error(std::string("Error loading YAML file '") + yaml_path + "': " + ex.what()));
    }
}


bool ProxyConfigLoader::asBool(const std::string &s) {
    return s == "1" || s == "true" || s == "TRUE" || s == "yes" || s == "on";
};


/**
 * @brief Applies an environment variable as a string to a JSON object at a given path.
 */
void ProxyConfigLoader::applyStringEnv(json &j, const json::json_pointer &path, const char *envVar) {
    if (auto v = getenvOpt(envVar)) {
        j[path] = *v;
    }
}

/**
 * @brief Applies an environment variable as a boolean to a JSON object at a given path.
 */
void ProxyConfigLoader::applyBoolEnv(json &j, const json::json_pointer &path, const char *envVar) {
    if (auto v = getenvOpt(envVar)) {
        j[path] = asBool(*v);
    }
}

/**
 * @brief Applies an environment variable as an integer to a JSON object at a given path.
 */
void ProxyConfigLoader::applyIntEnv(json &j, const json::json_pointer &path, const char *envVar) {
    if (auto v = getenvOpt(envVar)) {
        try {
            j[path] = std::stoi(*v);
        } catch (const std::exception &e) {
            // Optional: Add logging for invalid integer values
            // std::cerr << "Warning: Could not parse env var '" << envVar << "' as integer: " << *v << std::endl;
        }
    }
}


// --- Refactored applyEnvOverrides Function ---

void ProxyConfigLoader::applyEnvOverrides(json &j) {
    // ---------- frontend ----------
    applyBoolEnv(j, "/frontend/enable_http"_json_pointer, "FRONTEND_ENABLE_HTTP");
    applyBoolEnv(j, "/frontend/enable_https"_json_pointer, "FRONTEND_ENABLE_HTTPS");
    applyIntEnv(j, "/frontend/http_listen_port"_json_pointer, "FRONTEND_HTTP_PORT");
    applyIntEnv(j, "/frontend/https_listen_port"_json_pointer, "FRONTEND_HTTPS_PORT");

    // ---------- frontend.tls ----------
    applyStringEnv(j, "/frontend/tls/cert_file"_json_pointer, "FRONTEND_TLS_CERT_FILE");
    applyStringEnv(j, "/frontend/tls/key_file"_json_pointer, "FRONTEND_TLS_KEY_FILE");
    applyStringEnv(j, "/frontend/tls/key_pass_file"_json_pointer, "FRONTEND_TLS_KEY_PASS_FILE");
    applyStringEnv(j, "/frontend/tls/ca_file"_json_pointer, "FRONTEND_TLS_CA_FILE");

    // ---------- facilitator ----------
    applyStringEnv(j, "/facilitator/type"_json_pointer, "FACILITATOR_TYPE");
    applyStringEnv(j, "/facilitator/base_url"_json_pointer, "FACILITATOR_BASE_URL");
    applyStringEnv(j, "/facilitator/api_key_file"_json_pointer, "FACILITATOR_API_KEY_FILE");
}


// ---------- Resolve secret files to actual values ----------
void ProxyConfigLoader::resolveSecrets(json &j) {
    const std::string dbFile = j["database"].value("passwordFile", "");
    const std::string jwtFile = j["jwt"].value("secretFile", "");

    j["database"]["password"] = readFileFirstLine(dbFile, /*fallback*/ "");
    j["jwt"]["secret"] = readFileFirstLine(jwtFile, /*fallback*/ "");
}

// ---------- JSON Schema validation ----------
void ProxyConfigLoader::validateJson(const json &j, const std::string &schema_path) {
    std::ifstream sf(schema_path);
    if (!sf.is_open()) throw std::runtime_error("Cannot open schema file: " + schema_path);
    json schema = json::parse(sf);

    json_validator validator;
    validator.set_root_schema(schema); // throws on invalid schema
    validator.validate(j); // throws on validation error
}


// ---------- Orchestrator ----------
ProxyConfig ProxyConfigLoader::load(const std::string &yaml_path,
                                    const std::string &schema_path) {
    json j = yamlToJson(yaml_path);
    applyEnvOverrides(j);
    resolveSecrets(j);
    validateJson(j, schema_path);
    return toProxyConfig(j);
}

ProxyConfig ProxyConfigLoader::toProxyConfig(const nlohmann::json &j) {
    ProxyConfig config;
    // FrontEndConfig
    const auto &jf = j.at("frontend");
    config.frontEnd.httpEnabled = jf.at("enable_http").get<bool>();
    config.frontEnd.httpsEnabled = jf.at("enable_https").get<bool>();
    config.frontEnd.httpPort = jf.at("http_listen_port").get<uint16_t>();
    config.frontEnd.httpsPort = jf.at("https_listen_port").get<uint16_t>();
    const auto &jt = jf.at("tls");
    config.frontEnd.tls.certFile = jt.at("cert_file").get<std::string>();
    config.frontEnd.tls.keyFile = jt.at("key_file").get<std::string>();
    config.frontEnd.tls.keyPassFile = jt.at("key_pass_file").get<std::string>();
    if (jt.contains("ca_file") && !jt.at("ca_file").is_null())
        config.frontEnd.tls.caFile = jt.at("ca_file").get<std::string>();
    else
        config.frontEnd.tls.caFile = std::nullopt;

    // FacilitatorConfig
    const auto &jfaci = j.at("facilitator");
    config.facilitator.type = jfaci.at("type").get<std::string>();
    config.facilitator.baseUrl = jfaci.at("base_url").get<std::string>();
    if (jfaci.contains("api_key_file") && !jfaci.at("api_key_file").is_null())
        config.facilitator.apiKeyFile = jfaci.at("api_key_file").get<std::string>();
    else
        config.facilitator.apiKeyFile = std::nullopt;

    return config;
}
