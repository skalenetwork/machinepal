#include "MachinePayCommon.h"
#include <openssl/evp.h>
#include "CryptoManager.h"
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <utility>
#include <openssl/core_names.h>
#include <memory>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include "EthPrivateKey.h"
#include "EthAddress.h"
#include "Keccak.h"


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

EthAddress CryptoManager::deriveAddressFromPrivateKey(const EthPrivateKey& key) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) throw std::runtime_error("Failed to create EC_GROUP");
    BN_CTX* bnCtx = BN_CTX_new();
    if (!bnCtx) { EC_GROUP_free(group); throw std::runtime_error("Failed to create BN_CTX"); }
    BIGNUM* priv = BN_bin2bn(key.bytes().data(), 32, nullptr);
    if (!priv) { BN_CTX_free(bnCtx); EC_GROUP_free(group); throw std::runtime_error("Failed to create BIGNUM for private key"); }
    EC_POINT* pub = EC_POINT_new(group);
    if (!pub) { BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group); throw std::runtime_error("Failed to create EC_POINT"); }
    if (EC_POINT_mul(group, pub, priv, nullptr, nullptr, bnCtx) != 1) {
        EC_POINT_free(pub); BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group); throw std::runtime_error("EC_POINT_mul failed"); }

    BIGNUM* x = BN_new(); BIGNUM* y = BN_new();
    if (!x || !y) { if (x) BN_free(x); if (y) BN_free(y); EC_POINT_free(pub); BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group); throw std::runtime_error("BN_new failed"); }
    if (EC_POINT_get_affine_coordinates(group, pub, x, y, bnCtx) != 1) {
        BN_free(x); BN_free(y); EC_POINT_free(pub); BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group); throw std::runtime_error("EC_POINT_get_affine_coordinates failed"); }

    std::array<uint8_t,64> pubBytes{}; // uncompressed without prefix
    BN_bn2binpad(x, pubBytes.data(), 32);
    BN_bn2binpad(y, pubBytes.data()+32, 32);

    auto hash = keccak::keccak256(std::span<const uint8_t>(pubBytes.data(), 64));
    EthAddress address(hash.data()+12, 20);

    BN_free(x); BN_free(y); EC_POINT_free(pub); BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group);
    return address;
}

std::pair<EthPrivateKey, EthAddress>
CryptoManager::generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair() {
    auto pkeyDeleter = [](EVP_PKEY *p) { EVP_PKEY_free(p); };
    auto pctxDeleter = [](EVP_PKEY_CTX *p) { EVP_PKEY_CTX_free(p); };

    std::unique_ptr<EVP_PKEY_CTX, decltype(pctxDeleter)> pctx(
        EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr),
        pctxDeleter);
    if (!pctx) throw std::runtime_error("Failed to create EVP_PKEY_CTX");

    if (EVP_PKEY_keygen_init(pctx.get()) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen_init failed");

    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, (char*)"secp256k1", 0);
    params[1] = OSSL_PARAM_construct_end();
    if (EVP_PKEY_CTX_set_params(pctx.get(), params) <= 0)
        throw std::runtime_error("Failed to set EC curve params");

    EVP_PKEY *pkeyRaw = nullptr;
    if (EVP_PKEY_keygen(pctx.get(), &pkeyRaw) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen failed");
    std::unique_ptr<EVP_PKEY, decltype(pkeyDeleter)> pkey(pkeyRaw, pkeyDeleter);

    unsigned char privKey[32]; size_t privLen = sizeof(privKey);
    if (EVP_PKEY_get_raw_private_key(pkey.get(), privKey, &privLen) <= 0 || privLen != 32)
        throw std::runtime_error("Failed to get raw private key");

    unsigned char pubKey[65]; size_t pubLen = sizeof(pubKey);
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pubKey, &pubLen) <= 0 || pubLen != 65)
        throw std::runtime_error("Failed to get raw public key");

    // Skip 0x04 prefix, keccak hash of 64 bytes
    auto hash = keccak::keccak256(std::span<const uint8_t>(pubKey + 1, 64));

    EthPrivateKey privateKey(privKey, 32);
    EthAddress address(hash.data() + 12, 20);
    return std::make_pair(privateKey, address);
}