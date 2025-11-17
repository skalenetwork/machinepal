#pragma once
#include "crypto/EthPrivateKey.h"

class ProjectGenerator {

    // Constants for directory and file names
    static constexpr auto kSecretsDir = "secrets";
    static constexpr auto kCertsDir = "certs";
    static constexpr auto kWalletFile = "machinepay_wallet.key";
    static constexpr auto kCertFile = "machinepay_tls_certificate.crt";
    static constexpr auto kCertKeyFile = "machinepay_tls_certificate.key";

    // Private helper methods
    static void generateDirectoryStructure(const std::filesystem::path& baseDir);
    static EthPrivateKey generateWallet(const std::filesystem::path& baseDir);
    static void generateTLSCertificate(const std::filesystem::path& baseDir);
    static void generateConfiguration(const std::filesystem::path& baseDir,
                               EthPrivateKey& machinePayKey);

public:

    static void generateProject(const std::filesystem::path &baseDir);

};
