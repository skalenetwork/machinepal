#include "EIP3009.h"
#include "crypto/Keccak.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <sstream>
#include <iomanip>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Helper to encode and hash the authorization message
static std::array<uint8_t, 32> hashAuthorization(const std::string& from,
                                              const std::string& to,
                                              uint64_t value,
                                              uint64_t validAfter,
                                              uint64_t validBefore,
                                              const std::string& nonce) {
    // Concatenate and encode fields as per EIP-3009
    std::string message = from + to + std::to_string(value) +
                          std::to_string(validAfter) + std::to_string(validBefore) + nonce;
    return keccak::keccak256(message);
}

// Helper to convert bytes to hex string
static std::string bytesToHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return oss.str();
}

std::string EIP3009::signAuthorization(const std::string& from,
                                       const std::string& to,
                                       uint64_t value,
                                       uint64_t validAfter,
                                       uint64_t validBefore,
                                       const std::string& nonce,
                                       const EthPrivateKey& privateKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!ec_key) throw std::runtime_error("Failed to create EC_KEY");
    BIGNUM* priv_bn = BN_bin2bn(privateKey.bytes().data(), 32, nullptr);
    if (!EC_KEY_set_private_key(ec_key, priv_bn)) {
        BN_free(priv_bn);
        EC_KEY_free(ec_key);
        throw std::runtime_error("Failed to set private key");
    }
    unsigned int sig_len = ECDSA_size(ec_key);
    std::vector<unsigned char> sig(sig_len);
    if (!ECDSA_sign(0, hash.data(), hash.size(), sig.data(), &sig_len, ec_key)) {
        BN_free(priv_bn);
        EC_KEY_free(ec_key);
        throw std::runtime_error("ECDSA_sign failed");
    }
    BN_free(priv_bn);
    EC_KEY_free(ec_key);
    return bytesToHex(sig.data(), sig_len);
}

bool EIP3009::verifyAuthorization(const std::string& from,
                                  const std::string& to,
                                  uint64_t value,
                                  uint64_t validAfter,
                                  uint64_t validBefore,
                                  const std::string& nonce,
                                  const std::string& signature,
                                  const EthPublicKey& publicKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!ec_key) throw std::runtime_error("Failed to create EC_KEY");
    // Set public key
    const auto& pub_bytes = publicKey.bytes();
    EC_POINT* pub_point = EC_POINT_new(EC_KEY_get0_group(ec_key));
    if (!EC_POINT_oct2point(EC_KEY_get0_group(ec_key), pub_point, pub_bytes.data(), pub_bytes.size(), nullptr)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        throw std::runtime_error("Failed to set public key");
    }
    if (!EC_KEY_set_public_key(ec_key, pub_point)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        throw std::runtime_error("Failed to set public key");
    }
    EC_POINT_free(pub_point);
    // Convert signature hex to bytes
    std::vector<unsigned char> sig_bytes;
    for (size_t i = 0; i < signature.size(); i += 2) {
        sig_bytes.push_back(static_cast<unsigned char>(std::stoi(signature.substr(i, 2), nullptr, 16)));
    }
    int verify_status = ECDSA_verify(0, hash.data(), hash.size(), sig_bytes.data(), sig_bytes.size(), ec_key);
    EC_KEY_free(ec_key);
    return verify_status == 1;
}

