#include "ProxiConfigLoader.h"

#include "config/config.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <nlohmann/json-schema.hpp>
using nlohmann::json;
using nlohmann::json_schema::json_validator;

// ---------- tiny utils ----------
std::optional<std::string> ConfigLoader::getenvOpt(const char* key) {
  if (const char* v = std::getenv(key)) return std::string(v);
  return std::nullopt;
}

std::string ConfigLoader::readFileFirstLine(const std::string& path,
                                            const std::string& fallback) {
  std::ifstream f(path);
  if (!f.is_open()) return fallback;
  std::string line;
  std::getline(f, line);
  return line;
}

// ---------- YAML -> JSON (recursive) ----------
static json yamlNodeToJson(const YAML::Node& node) {
  using Type = YAML::NodeType::value;
  switch (node.Type()) {
    case Type::Null:   return nullptr;
    case Type::Scalar: return node.as<std::string>();
    case Type::Sequence: {
      json arr = json::array();
      for (auto&& it : node) arr.push_back(yamlNodeToJson(it));
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

json ConfigLoader::yamlToJson(const std::string& yaml_path) {
  YAML::Node root = YAML::LoadFile(yaml_path);
  return yamlNodeToJson(root);
}

// ---------- ENV overlay (pick high-value knobs) ----------
void ConfigLoader::applyEnvOverrides(json& j) {
  // Server
  if (auto v = getenvOpt("SERVER_HOST")) j["server"]["host"] = *v;
  if (auto v = getenvOpt("SERVER_PORT")) j["server"]["port"] = std::stoi(*v);

  // Database scalars
  if (auto v = getenvOpt("DATABASE_HOST")) j["database"]["host"] = *v;
  if (auto v = getenvOpt("DATABASE_PORT")) j["database"]["port"] = std::stoi(*v);
  if (auto v = getenvOpt("DATABASE_USER")) j["database"]["user"] = *v;

  // Secret *file* paths (do not pass raw secrets in env if you can avoid it)
  if (auto v = getenvOpt("DATABASE_PASSWORD_FILE")) j["database"]["passwordFile"] = *v;
  if (auto v = getenvOpt("JWT_SECRET_FILE")) j["jwt"]["secretFile"] = *v;
}

// ---------- Resolve secret files to actual values ----------
void ConfigLoader::resolveSecrets(json& j) {
  const std::string dbFile = j["database"].value("passwordFile", "");
  const std::string jwtFile = j["jwt"].value("secretFile", "");

  j["database"]["password"] = readFileFirstLine(dbFile, /*fallback*/ "");
  j["jwt"]["secret"] = readFileFirstLine(jwtFile, /*fallback*/ "");
}

// ---------- JSON Schema validation ----------
void ConfigLoader::validateJson(const json& j, const std::string& schema_path) {
  std::ifstream sf(schema_path);
  if (!sf.is_open()) throw std::runtime_error("Cannot open schema file: " + schema_path);
  json schema = json::parse(sf);

  json_validator validator;
  validator.set_root_schema(schema); // throws on invalid schema
  validator.validate(j);             // throws on validation error
}

// ---------- Materialize strongly-typed config ----------
AppConfig ConfigLoader::toAppConfig(const json& j) {
  AppConfig cfg{};
  cfg.server_host = j["server"].value("host", "0.0.0.0");
  cfg.server_port = j["server"].value("port", 8080);

  cfg.db_host = j["database"].value("host", "localhost");
  cfg.db_port = j["database"].value("port", 5432);
  cfg.db_user = j["database"].value("user", "appuser");
  cfg.db_password = j["database"].value("password", "");
  cfg.jwt_secret = j["jwt"].value("secret", "");

  return cfg;
}

// ---------- Orchestrator ----------
AppConfig ConfigLoader::load(const std::string& yaml_path,
                             const std::string& schema_path) {
  json j = yamlToJson(yaml_path);
  applyEnvOverrides(j);
  resolveSecrets(j);
  validateJson(j, schema_path);
  return toAppConfig(j);
}
