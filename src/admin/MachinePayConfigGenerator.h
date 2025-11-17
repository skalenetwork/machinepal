#pragma once
#include <filesystem>


class EthPrivateKey;

class MachinePayConfigGenerator {
public:
    // Creates machinepay.yml inside dirPath (does not overwrite existing file).
    static void generateDefaultConfig(const std::filesystem::path &dirPath, EthPrivateKey& machinePayKey);
};

