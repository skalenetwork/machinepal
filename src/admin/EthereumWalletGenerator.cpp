#include "MachinePayCommon.h"
#include "crypto/EthPrivateKey.h"
#include "EthereumWalletGenerator.h"
#include <fstream>

void EthereumWalletGenerator::generateWalletFile(const std::filesystem::path& privKeyPath) {
    try {
        auto privateKey = EthPrivateKey::generate();

        if (std::filesystem::exists(privKeyPath)) {
            throw std::runtime_error("File already exists. Refusing to overwrite: "
                + privKeyPath.string());
        }

        // Create parent directories if they don't exist
        if (privKeyPath.has_parent_path()) {
            std::filesystem::create_directories(privKeyPath.parent_path());
        }

        // Open the file using the path object directly (preferred C++17)
        std::ofstream f(privKeyPath);
        if (!f) {
            throw std::runtime_error("Failed to open file for writing: " + privKeyPath.string());
        }

        // Write the data
        f << privateKey.toHex() << '\n';
        if (!f) {
            // This checks for a write error (e.g., disk full)
            throw std::runtime_error("Failed to write data to file: " + privKeyPath.string());
        }

        f.close(); // Close the file before changing permissions

        // --- Corrected Permissions ---
        std::error_code ec;
        std::filesystem::permissions(
            privKeyPath,
            // Use the 'perms' enum
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            // Use 'replace' to set permissions *exactly* (removes group/other)
            std::filesystem::perm_options::replace,
            ec
        );

        if (ec) {
            throw std::filesystem::filesystem_error(
                "Failed to set permissions on new wallet file: " + privKeyPath.string(), ec);
        }

    } catch (...) {
        // Use standard C++ nested exceptions
        std::throw_with_nested(std::runtime_error(
            "Failed to generate Ethereum wallet file at: " + privKeyPath.string()
        ));
    }
}
