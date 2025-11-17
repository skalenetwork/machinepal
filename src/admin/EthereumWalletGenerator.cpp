#include "MachinePayCommon.h"
#include "crypto/EthPrivateKey.h"
#include "EthereumWalletGenerator.h"
#include <fstream>

void EthereumWalletGenerator::generateWalletFile(const boost::filesystem::path& privKeyPath) {
    try {
        auto privateKey = EthPrivateKey::generate();

        if (boost::filesystem::exists(privKeyPath)) {
            throw std::runtime_error("File already exists. Refusing to overwrite, delete file first and retry: "
                + privKeyPath.string());
        }

        if (privKeyPath.has_parent_path())
            boost::filesystem::create_directories(privKeyPath.parent_path());
        std::ofstream f(privKeyPath.string(), std::ios::out);
        if (!f) {
            throw runtime_error("Failed to open file for writing." + privKeyPath.string());
        }

        f << privateKey.toHex() << '\n';

        if (!f) {
            std::string errorMsg = "Failed to open file for writing: " + privKeyPath.string();

            if (errno != 0) {
                // This gives the OS-level reason
                errorMsg += ". System error (" + std::to_string(errno) + "): " + std::string(strerror(errno));
            } else {
                errorMsg += ". (No specific system error code available)";
            }

            // You can also add more pre-checks for common issues
            if (privKeyPath.has_parent_path()) {
                auto parent = privKeyPath.parent_path();
                if (!boost::filesystem::exists(parent)) {
                    errorMsg += ". Diagnosis: Parent directory does not exist.";
                } else if (!boost::filesystem::is_directory(parent)) {
                    errorMsg += ". Diagnosis: Parent path is a file, not a directory.";
                }
            }

            throw std::runtime_error(errorMsg);
        }

        f.close(); // Close before setting permissions

        boost::filesystem::permissions(privKeyPath, boost::filesystem::owner_read | boost::filesystem::owner_write);
    } catch (...) {
        RETHROW_NESTED2("Failed to generate Ethereum wallet file");
    }
};
