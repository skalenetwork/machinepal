#include "MachinePayCommon.h"
#include "MachinePayConfigGenerator.h"

#include "crypto/EthPrivateKey.h"

static const char* kYamlTemplate = R"(# Machinepay server configuration. May be overridden by environment variables.
server:
  hostname: localhost
  http:
    enable: false
    port: 8080
  https:
    enable: false
    port: 8443
    cert_file: certs/machinepay_tls_certificate.tls
    key_file: secrets/machinepay_tls_certificate.key

network:
  name: machinepay-easynet
  wallet_address: {}

log:
  # Optional Log verbosity level — one of: trace, debug, info, warn, error, fatal
  # Default: info
  level: trace

resources:
  - name: hello_world
    type: local_file
    location: resources/hello_world.txt
    price: 1.545
    token: USDC

  - name: hello_world_api_rest
    type: api-rest
    location: https://jsonplaceholder.typicode.com/posts/1
    price: 12
    token: USDC
)";

void MachinePayConfigGenerator::generateDefaultConfig(const std::filesystem::path& dirPath,  EthPrivateKey& machinePayKey) {
    try {
        if (!dirPath.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(dirPath, ec);

            if (ec) {
                throw std::filesystem::filesystem_error(
                    "Failed to create target directory for machinepay.yml", dirPath, ec);
            }
        }
        auto filePath = dirPath / "machinepay.yml";
        if (std::filesystem::exists(filePath)) {
            throw std::runtime_error("Config file already exists. Refusing to overwrite: " + filePath.string());
        }

        // This is already correct and modern C++17
        std::ofstream out(filePath);
        if (!out) {
            throw std::runtime_error("Failed to open config file for writing: " + filePath.string());
        }

        std::string finalYaml = folly::sformat(
            kYamlTemplate,
            machinePayKey.computePublicKey().getAddress().toHex(PREFIX_0x)
        );

        out << finalYaml;
        if (!out) {
            out.close();
            std::error_code ec;
            std::filesystem::remove(filePath, ec);
            throw std::runtime_error("Failed to write YAML content to: " + filePath.string());
        }
        out.close();

        // This permissions call is already correct
        std::error_code permEc;
        std::filesystem::permissions(
            filePath,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace,
            permEc
        );

        if (permEc) {
            // --- Fix 2: Correct Error Code Handling ---
            // Don't pass the permissions error code (permEc) to remove().
            // Use a new error code for the remove operation.
            std::error_code removeEc;
            std::filesystem::remove(filePath, removeEc); // Try to clean up

            // Throw the *original* and important error
            throw std::filesystem::filesystem_error(
                "Successfully wrote config but failed to set secure permissions", filePath, permEc);
        }
    } catch (...) {
        // This is already correct
        std::throw_with_nested(std::runtime_error("Failed to generate machinepay.yml in directory: " + dirPath.string()));
    }
}