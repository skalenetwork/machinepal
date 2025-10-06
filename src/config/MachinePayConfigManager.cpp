#include "MachinePayConfigManager.h"

#include "common.h"
#include "MachinePayConfigLoader.h"
#include "MachinePayConfig.h"
#include "config/MachinePayConfigSchema.h"
#include "init/InitLibs.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
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



// ---------- Orchestrator ----------
std::shared_mutex MachinePayConfigManager::latestConfigMutex_;
std::shared_ptr<MachinePayConfig> MachinePayConfigManager::latestConfig_ = nullptr;
std::chrono::system_clock::time_point MachinePayConfigManager::latestConfigMTime_ = {};
std::string MachinePayConfigManager::latestConfigSha256_ = "";

namespace {
std::string computeSha256(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return "";
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    const EVP_MD* md = EVP_sha256();
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    char buf[4096];
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if (file.gcount() > 0) {
            EVP_DigestUpdate(ctx, buf, file.gcount());
        }
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}
}

void MachinePayConfigManager::loadConfig(const std::string &yaml_path) {
    namespace fs = std::filesystem;
    std::unique_lock<std::shared_mutex> lock(latestConfigMutex_);
    auto hash = computeSha256(yaml_path);

    if (hash == latestConfigSha256_) {
        return;
    }
    latestConfig_ = MachinePayConfigLoader::loadConfig(yaml_path);
    // Record last modified time
    try {
        auto ftime = fs::last_write_time(yaml_path);
        latestConfigMTime_ = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                ftime.time_since_epoch()
            )
        );
    } catch (...) {
        latestConfigMTime_ = std::chrono::system_clock::time_point{};
    }
    // Record SHA256

}


std::shared_ptr<MachinePayConfig> MachinePayConfigManager::latestConfig() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfig_;
}

std::chrono::system_clock::time_point MachinePayConfigManager::latestConfigMTime() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigMTime_;
}

const std::string& MachinePayConfigManager::latestConfigSha256() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigSha256_;
}

// Definition of the static member
