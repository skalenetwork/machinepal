#include "MachinePayConfigManager.h"

#include "common.h"
#include "MachinePayConfigLoader.h"
#include "MachinePayConfig.h"
#include "config/MachinePayConfigSchema.h"
#include "init/Init.h"
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

std::string MachinePayConfigManager::computeBlakeHash(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file for hashing: " + filePath);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");
    const EVP_MD *md = EVP_blake2b512();
    if (!md) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to get BLAKE2b-512 digest method");
    }
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    char buf[4096];
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if (file.bad()) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Error reading file during hashing: " + filePath);
        }
        if (file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx, buf, file.gcount()) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestUpdate failed");
            }
        }
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
    return oss.str();
}

void MachinePayConfigManager::checkExistsAndReadable(std::string configFile) {
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        throw std::runtime_error(
            "Config file '" + configFile + "' does not exist. Failed to get current working directory.");
    }

    // Check that configFile exists
    if (!std::filesystem::exists(configFile)) {
        throw std::runtime_error(
            "Config file '" + configFile + "' does not exist. Current working directory: " + std::string(cwd));
    }
    // Check that configFile is not a directory
    if (std::filesystem::is_directory(configFile)) {
        throw std::runtime_error(
            "Config file '" + configFile + "' is a directory, not a file. Current working directory: " +
            std::string(cwd));
    }
    // Check that configFile is readable
    std::ifstream configTest(configFile);
    if (!configTest.good()) {
        char cwd2[4096];
        if (!getcwd(cwd2, sizeof(cwd2))) {
            throw std::runtime_error(
                "Config file '" + configFile + "' is not readable. Failed to get current working directory.");
        }
        throw std::runtime_error(
            "Config file '" + configFile + "' is not readable. Current working directory: " + std::string(cwd2));
    }
    configTest.close();
}

void MachinePayConfigManager::reloadConfigUnsafe() {
    try {
        CHECK_STATE(!configPath_.empty())

        checkExistsAndReadable(configPath_);

        auto hash = computeBlakeHash(configPath_);

        if (hash == latestConfigHash_) {
            CHECK_STATE(latestConfig_);
            return;
        }
        latestConfig_ = MachinePayConfigLoader::loadConfig(configPath_);
        // Record last modified time
        auto ftime = std::filesystem::last_write_time(configPath_);
        latestConfigModificationTime_ = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                ftime.time_since_epoch()
            )
        );
    } catch (const std::exception &ex) {
        RETHROW_NESTED("MachinePayConfigManager::reloadConfigUnsafe failed: ", ex);
    }

    CHECK_STATE(latestConfig_);
}

void MachinePayConfigManager::loadConfig(const std::string &yamlPath) {
    namespace fs = std::filesystem;
    std::unique_lock<std::shared_mutex> lock(latestConfigMutex_);
    configPath_ = yamlPath;
    reloadConfigUnsafe();
    CHECK_STATE(latestConfig_);
}

void MachinePayConfigManager::reloadConfig() {
    std::unique_lock<std::shared_mutex> lock(latestConfigMutex_);
    reloadConfigUnsafe();
    CHECK_STATE(latestConfig_);
}


std::shared_ptr<MachinePayConfig> MachinePayConfigManager::latestConfig() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    CHECK_STATE(latestConfig_);
    return latestConfig_;
}

std::chrono::system_clock::time_point MachinePayConfigManager::latestConfigModificationTime() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigModificationTime_;
}

const std::string &MachinePayConfigManager::latestConfigSha256() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigHash_;
}

// Definition of the static member


std::shared_mutex MachinePayConfigManager::latestConfigMutex_;
std::shared_ptr<MachinePayConfig> MachinePayConfigManager::latestConfig_ = nullptr;
std::chrono::system_clock::time_point MachinePayConfigManager::latestConfigModificationTime_ = {};
std::string MachinePayConfigManager::latestConfigHash_ = "";
std::string MachinePayConfigManager::configPath_ = "";
