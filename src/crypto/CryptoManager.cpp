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
#include "EthPublicKey.h"
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

EthPublicKey CryptoManager::derivePublicKeyFromPrivateKey(const EthPrivateKey &key) {
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

    BN_free(x); BN_free(y); EC_POINT_free(pub); BN_free(priv); BN_CTX_free(bnCtx); EC_GROUP_free(group);
    return EthPublicKey(pubBytes);
}

std::pair<EthPrivateKey, EthAddress>
CryptoManager::generateHardHatCompatibleEthereumPrivateKeyAndAddressAsPair() {
    EC_KEY* ec = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!ec) throw std::runtime_error("EC_KEY_new_by_curve_name failed");
    if (EC_KEY_generate_key(ec) != 1) { EC_KEY_free(ec); throw std::runtime_error("EC_KEY_generate_key failed"); }
    const BIGNUM* privBn = EC_KEY_get0_private_key(ec);
    if (!privBn) { EC_KEY_free(ec); throw std::runtime_error("Failed to get private key BIGNUM"); }

    std::array<uint8_t,32> privBytes{};
    if (BN_bn2binpad(privBn, privBytes.data(), 32) != 32) { EC_KEY_free(ec); throw std::runtime_error("BN_bn2binpad private failed"); }

    const EC_GROUP* group = EC_KEY_get0_group(ec);
    const EC_POINT* pubPoint = EC_KEY_get0_public_key(ec);
    if (!group || !pubPoint) { EC_KEY_free(ec); throw std::runtime_error("Failed to get public key point"); }

    BIGNUM* x = BN_new(); BIGNUM* y = BN_new();
    if (!x || !y) { if (x) BN_free(x); if (y) BN_free(y); EC_KEY_free(ec); throw std::runtime_error("BN_new failed"); }
    if (EC_POINT_get_affine_coordinates(group, pubPoint, x, y, nullptr) != 1) { BN_free(x); BN_free(y); EC_KEY_free(ec); throw std::runtime_error("EC_POINT_get_affine_coordinates failed"); }

    std::array<uint8_t,64> pubBytes{};
    BN_bn2binpad(x, pubBytes.data(), 32);
    BN_bn2binpad(y, pubBytes.data()+32, 32);

    auto hash = keccak::keccak256(std::span<const uint8_t>(pubBytes.data(), 64));
    EthPrivateKey privateKey(privBytes);
    EthAddress address(hash.data()+12, 20);

    BN_free(x); BN_free(y); EC_KEY_free(ec);
    return { privateKey, address };
}