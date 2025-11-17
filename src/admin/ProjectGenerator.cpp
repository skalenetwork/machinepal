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
        FolderGenerator fg(baseDir);
        fg.generateFolderStructure();

        // 1. Generate Ethereum private key and store via EthereumWalletGenerator
        auto machinePayKey = EthPrivateKey::generate();
        const auto walletPath = baseDir / "secrets" / "machinepay.key";
        EthereumWalletGenerator walletGen;
        walletGen.generateWalletFileFromKey(walletPath, machinePayKey);

        // 2. Generate TLS certificate and private key
        TLSCertGenerator tlsGen;
        const auto certPath = baseDir / "certs" / "machinepay_tls_certificate.tls";
        const auto certKeyPath = baseDir / "secrets" / "machinepay_tls_certificate.key";
        tlsGen.generateDefaultCertFiles(certPath, certKeyPath);

        // 3. Generate machinepay.yml (uses wallet address)
        MachinePayConfigGenerator cfgGen;
        cfgGen.generateDefaultConfig(baseDir, machinePayKey);

    } catch (...) {
        std::throw_with_nested(std::runtime_error(
            "ProjectGenerator::generateProject failed for base directory: " + baseDir.string()));
    }
}
