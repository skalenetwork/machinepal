#pragma once
#include <string>
#include <utility>
#include "EthPrivateKey.h"
#include "Address.h"


class CryptoManager {

public:
    /**
     * Computes the BLAKE2b-512 hash of the file at the given path.
     * @param filePath Path to the file to hash.
     * @return Hex string of the BLAKE2b-512 hash.
     */
    static std::string computeBlakeHash(const std::string& filePath);

    /**
     * Generates a random Ethereum private key and address compatible with HardHat.
     * @return Pair of EthPrivateKey and Address objects.
     */
    static std::pair<EthPrivateKey, Address> generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair();
};
