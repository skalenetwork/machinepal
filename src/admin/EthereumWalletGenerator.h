#pragma once

#include <string>
#include <boost/filesystem/path.hpp>



class EthPrivateKey; // forward declaration

class EthereumWalletGenerator {
public:
    static void generateWalletFileFromKey(const std::filesystem::path &privKeyPath, const EthPrivateKey &key);
};
