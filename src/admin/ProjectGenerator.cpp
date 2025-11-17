#include "MachinePayCommon.h"
#include "ProjectGenerator.h"
#include "FolderGenerator.h"
#include "EthereumWalletGenerator.h"
#include "TLSCertGenerator.h"
#include "MachinePayConfigGenerator.h"
#include "crypto/EthPrivateKey.h"
#include <stdexcept>


void ProjectGenerator::generateProject(const std::filesystem::path& baseDir) {
    try {
        // Each step is now a clear, self-documenting function call
        generateDirectoryStructure(baseDir);

        // 1. Generate wallet (returns key for use in config)
        auto machinePayKey = generateWallet(baseDir);

        // 2. Generate TLS certificate
        generateTLSCertificate(baseDir);

        // 3. Generate configuration
        generateConfiguration(baseDir, machinePayKey);

    } catch (...) {
        // The nested exception provides context for where the failure occurred.
        std::throw_with_nested(std::runtime_error(
            "Failed to generate project in directory: " + baseDir.string()));
    }
}


void ProjectGenerator::generateDirectoryStructure(const std::filesystem::path& baseDir) {
    FolderGenerator fg(baseDir);
    fg.generateFolderStructure();
}

EthPrivateKey ProjectGenerator::generateWallet(const std::filesystem::path& baseDir) {
    auto machinePayKey = EthPrivateKey::generate();

    // Use constants for paths
    const auto walletPath = baseDir / kSecretsDir / kWalletFile;

    EthereumWalletGenerator walletGen;
    walletGen.generateWalletFileFromKey(walletPath, machinePayKey);

    return machinePayKey;
}


void ProjectGenerator::generateTLSCertificate(const std::filesystem::path& baseDir) {
    // Use constants for paths
    const auto certPath = baseDir / kCertsDir / kCertFile;
    const auto certKeyPath = baseDir / kSecretsDir / kCertKeyFile;

    TLSCertGenerator tlsGen;
    tlsGen.generateDefaultCertFiles(certPath, certKeyPath);
}


void ProjectGenerator::generateConfiguration(const std::filesystem::path& baseDir,
                                           EthPrivateKey& machinePayKey) {
    MachinePayConfigGenerator cfgGen;
    cfgGen.generateDefaultConfig(baseDir, machinePayKey);
}