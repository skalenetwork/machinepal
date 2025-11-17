#pragma once

#include <string>
#include <boost/filesystem/path.hpp>

struct EthereumWallet {
    std::string privateKeyHex;
    std::string publicKeyHex;  // Uncompressed (0x04 + X + Y) hex
    std::string addressHex;    // 0x + 40 hex chars
};

class EthereumWalletGenerator {
public:
    void generateWalletFile(const boost::filesystem::path& privKeyPath;

};
