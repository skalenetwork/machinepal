#include "MachinePayCommon.h"
#include <openssl/evp.h>
#include "CryptoManager.h"
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <utility>
#include <openssl/core_names.h>
#include <memory>
#include "EthPrivateKey.h"
#include "Address.h"


std::string CryptoManager::computeBlakeHash(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file for hashing: " + filePath);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    const EVP_MD *md = EVP_blake2b512();
    if (!md) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to get BLAKE2b-512 digest method");
    }
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    char buf[4096];
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if (file.bad()) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Error reading file during hashing: " + filePath);
        }
        if (file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx, buf, file.gcount()) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestUpdate failed");
            }
        }
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

// Helper to convert bytes to hex string
static std::string toHex(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
        oss << std::setw(2) << (int)data[i];
    return oss.str();
}

std::pair<EthPrivateKey, Address>
CryptoManager::generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair() {
    // Define custom deleters for OpenSSL objects to ensure cleanup
    auto pkeyDeleter = [](EVP_PKEY *p) {
        EVP_PKEY_free(p);
    };
    auto pctxDeleter = [](EVP_PKEY_CTX *p) {
        EVP_PKEY_CTX_free(p);
    };
    auto mdctxDeleter = [](EVP_MD_CTX *p) {
        EVP_MD_CTX_free(p);
    };

    // Generate secp256k1 keypair using EVP_PKEY and OSSL_PARAM (OpenSSL 3.x compatible)
    std::unique_ptr<EVP_PKEY_CTX, decltype(pctxDeleter)> pctx(
        EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL),
        pctxDeleter);
    if (!pctx)
        throw std::runtime_error("Failed to create EVP_PKEY_CTX");

    if (EVP_PKEY_keygen_init(pctx.get()) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen_init failed");

    OSSL_PARAM params[2];
    params[0] =
        OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, (char *)"secp256k1", 0);
    params[1] = OSSL_PARAM_construct_end();
    if (EVP_PKEY_CTX_set_params(pctx.get(), params) <= 0)
        throw std::runtime_error("Failed to set EC curve params");

    EVP_PKEY *pkeyRaw = NULL;
    if (EVP_PKEY_keygen(pctx.get(), &pkeyRaw) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen failed");
    std::unique_ptr<EVP_PKEY, decltype(pkeyDeleter)> pkey(pkeyRaw, pkeyDeleter);

    // Extract raw private key
    unsigned char privKey[32];
    size_t        privLen = sizeof(privKey);
    if (EVP_PKEY_get_raw_private_key(pkey.get(), privKey, &privLen) <= 0 || privLen != 32)
        throw std::runtime_error("Failed to get raw private key");

    // Extract raw public key (uncompressed)
    unsigned char pubKey[65];
    size_t        pubLen = sizeof(pubKey);
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pubKey, &pubLen) <= 0 || pubLen != 65)
        throw std::runtime_error("Failed to get raw public key");

    // Keccak-256 hash (OpenSSL SHA3)
    std::unique_ptr<EVP_MD_CTX, decltype(mdctxDeleter)> mdctx(EVP_MD_CTX_new(), mdctxDeleter);
    if (!mdctx)
        throw std::runtime_error("Failed to create EVP_MD_CTX");

    if (EVP_DigestInit_ex(mdctx.get(), EVP_sha3_256(), NULL) <= 0)
        throw std::runtime_error("EVP_DigestInit_ex failed");
    if (EVP_DigestUpdate(mdctx.get(), pubKey + 1, 64) <= 0)
        throw std::runtime_error("EVP_DigestUpdate failed");
    // skip 0x04 prefix
    unsigned char hash[32];
    unsigned int  hashLen;
    if (EVP_DigestFinal_ex(mdctx.get(), hash, &hashLen) <= 0)
        throw std::runtime_error("EVP_DigestFinal_ex failed");

    // Create EthPrivateKey and Address objects from the raw bytes
    EthPrivateKey privateKey(privKey, 32);
    Address       address(hash + 12, 20);

    return std::make_pair(privateKey, address);
}