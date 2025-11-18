#include "MachinePayCommon.h"
#include "ProjectGenerator.h"
#include "FolderGenerator.h"
#include "EthereumWalletGenerator.h"
#include "TLSCertGenerator.h"
#include "MachinePayConfigGenerator.h"
#include "crypto/EthPrivateKey.h"
#include <stdexcept>
#include <filesystem>


void ProjectGenerator::generateProjectInCurrentWorkingDir() {
    spdlog::info("Initializing project in the current directory...");
    auto cwd = std::filesystem::current_path();
    generateProject(cwd);
    spdlog::info("Project initialized successfully.");
}

void ProjectGenerator::generateProject(const std::filesystem::path &baseDir) {
    validateBaseDirEmpty(baseDir);

    // Each step is now a clear, self-documenting function call
    generateDirectoryStructure(baseDir);

    // 1. Generate wallet (returns key for use in config)
    auto machinePayKey = generateWallet(baseDir);

    // 2. Generate TLS certificate
    generateTLSCertificate(baseDir);

    // 3. Generate configuration
    generateConfiguration(baseDir, machinePayKey);
}


void ProjectGenerator::generateDirectoryStructure(const std::filesystem::path &baseDir) {
    FolderGenerator fg(baseDir);
    fg.generateFolderStructure();
}

EthPrivateKey ProjectGenerator::generateWallet(const std::filesystem::path &baseDir) {
    auto machinePayKey = EthPrivateKey::generate();

    // Use constants for paths
    const auto walletPath = baseDir / kSecretsDir / kWalletFile;

    EthereumWalletGenerator walletGen;
    walletGen.generateWalletFileFromKey(walletPath, machinePayKey);

    return machinePayKey;
}


void ProjectGenerator::generateTLSCertificate(const std::filesystem::path &baseDir) {
    // Use constants for paths
    const auto certPath = baseDir / kCertsDir / kCertFile;
    const auto certKeyPath = baseDir / kSecretsDir / kCertKeyFile;

    TLSCertGenerator tlsGen;
    tlsGen.generateDefaultCertFiles(certPath, certKeyPath);
}


void ProjectGenerator::generateConfiguration(const std::filesystem::path &baseDir,
                                             EthPrivateKey &machinePayKey) {
    MachinePayConfigGenerator cfgGen;
    cfgGen.generateDefaultConfig(baseDir, machinePayKey);
}


void ProjectGenerator::validateBaseDirEmpty(const std::filesystem::path &baseDir) {
    std::error_code ec;
    if (!std::filesystem::exists(baseDir, ec) || ec) {
        throw std::runtime_error("Project base directory does not exist: " + baseDir.string());
    }
    if (!std::filesystem::is_directory(baseDir, ec) || ec) {
        throw std::runtime_error("Project base path is not a directory: " + baseDir.string());
    }
    // Check emptiness
    auto it = std::filesystem::directory_iterator(baseDir, ec);
    if (ec) {
        throw std::runtime_error("Failed to read project base directory: " + baseDir.string());
    }
    if (it != std::filesystem::end(it)) {
        throw std::runtime_error(string("Directory is not empty. Run init in an empty directory."));
    }
}
