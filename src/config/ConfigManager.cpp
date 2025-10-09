#include "ConfigManager.h"

#include "common.h"
#include "ConfigLoader.h"
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


std::string ConfigManager::computeBlakeHash(const std::string &filePath) {
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

void ConfigManager::checkFileExistsAndReadable(const std::string& configFile) {
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


ptr<ConfigManager> ConfigManager::initManager(const std::map<std::string, std::string>& configValuesFromCliAndEnv) {
    try
    {
        auto instance = create(configValuesFromCliAndEnv);
        instance->reloadConfig();
        return instance;
    } catch (const std::exception &ex) {
        RETHROW_NESTED("MachinePayConfigManager::initManager failed: ");
    }
}

ptr<ConfigManager> ConfigManager::create(const std::map<std::string, std::string>& configValuesFromCliAndEnv) {
    ptr<ConfigManager> mgr(new ConfigManager());
    mgr->setConfigValuesFromCliAndEnv(configValuesFromCliAndEnv);
    return mgr;
}



void ConfigManager::setConfigValuesFromCliAndEnv(const std::map<std::string, std::string>& values) {
    configValuesFromCliAndEnv_ = values;
    CHECK_STATE(values.contains("CONFIG"));
    userProvidedConfigPath_ = values.at("CONFIG");
    CHECK_STATE(!userProvidedConfigPath_.empty())
    fullyResolvedConfigPath_ = FileManager::resolveCanonicalPathAgainstCwd(userProvidedConfigPath_);
    CHECK_STATE(!fullyResolvedConfigPath_.empty())
}

void ConfigManager::reloadConfig() {
    std::unique_lock<std::shared_mutex> lock(latestConfigMutex_);

    try {
        CHECK_STATE(!userProvidedConfigPath_.empty());
        CHECK_STATE(!fullyResolvedConfigPath_.empty());

        spdlog::info("Loading machinepay config from: {}", fullyResolvedConfigPath_);
        spdlog::info("All relative paths in the config will be resolved against the machinepay config location.");


        checkFileExistsAndReadable(fullyResolvedConfigPath_);

        auto hash = computeBlakeHash(userProvidedConfigPath_);

        if (hash == latestConfigHash_) {
            CHECK_STATE(latestConfig_);
            return;
        }

        ConfigLoader loader(ConfigManager::configValuesFromCliAndEnv_);

        latestConfig_ = loader.loadFromYamlFile(userProvidedConfigPath_);

        // Record last modified time
        auto ftime = std::filesystem::last_write_time(userProvidedConfigPath_);
        latestConfigModificationTime_ = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                ftime.time_since_epoch()
            )
        );
    } catch (const std::exception &ex) {
        RETHROW_NESTED("MachinePayConfigManager::reloadConfig failed: ");
    }

    CHECK_STATE(latestConfig_);
}


std::shared_ptr<MachinePayConfig> ConfigManager::latestConfig() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    CHECK_STATE(latestConfig_);
    return latestConfig_;
}

std::chrono::system_clock::time_point ConfigManager::latestConfigModificationTime() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigModificationTime_;
}

const std::string &ConfigManager::latestConfigSha256() {
    std::shared_lock<std::shared_mutex> lock(latestConfigMutex_);
    return latestConfigHash_;
}
