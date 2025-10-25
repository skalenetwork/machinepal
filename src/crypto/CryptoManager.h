#pragma once
#include <string>
#include <utility>
#include "EthPrivateKey.h"
#include "EthAddress.h"


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
    static std::pair<EthPrivateKey, EthAddress> generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair();

    /**
     * Derives the Ethereum address from the given private key.
     * @param key The private key to derive the address from.
     * @return The derived Ethereum address.
     */
    static EthAddress deriveAddressFromPrivateKey(const EthPrivateKey& key);
};
