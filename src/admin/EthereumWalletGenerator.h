#pragma once

#include <string>
#include <boost/filesystem/path.hpp>

struct EthereumWallet {
    std::string privateKeyHex;
    std::string publicKeyHex;  // Uncompressed (0x04 + X + Y) hex
    std::string addressHex;    // 0x + 40 hex chars
};

class EthPrivateKey; // forward declaration

class EthereumWalletGenerator {
public:
    void generateWalletFile(const std::filesystem::path& privKeyPath);
    void generateWalletFileFromKey(const std::filesystem::path& privKeyPath, const EthPrivateKey& key);
};
