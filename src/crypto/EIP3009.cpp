#include "EIP3009.h"
#include "crypto/Keccak.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <sstream>
#include <iomanip>
#include "EthAddress.h"
#include "EthSignature.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Helper to encode and hash the authorization message
static std::array<uint8_t, 32> hashAuthorization(const EthAddress& from,
                                              const EthAddress& to,
                                              uint64_t value,
                                              uint64_t validAfter,
                                              uint64_t validBefore,
                                              const std::string& nonce) {
    // Concatenate and encode fields as per EIP-3009
    std::string message = from.toHex() + to.toHex() + std::to_string(value) +
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

EthSignature EIP3009::signAuthorization(const EthAddress& from,
                                       const EthAddress& to,
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
    // Pad/copy to 65 bytes, set v to 0 (no recovery, for compatibility)
    std::array<uint8_t, 65> sigArr{};
    size_t copyLen = std::min<size_t>(sig_len, 64);
    std::copy(sig.begin(), sig.begin() + copyLen, sigArr.begin());
    sigArr[64] = 0; // v value (could be set to 27/28 if recovery is implemented)
    return EthSignature(sigArr);
}

bool EIP3009::verifyAuthorization(const EthAddress& from,
                                  const EthAddress& to,
                                  uint64_t value,
                                  uint64_t validAfter,
                                  uint64_t validBefore,
                                  const std::string& nonce,
                                  const EthSignature& signature,
                                  const EthPublicKey& publicKey) {
    auto hash = hashAuthorization(from, to, value, validAfter, validBefore, nonce);
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!ec_key) throw std::runtime_error("Failed to create EC_KEY for secp256k1 curve");
    // Set public key
    const auto& pub_bytes = publicKey.bytes();
    EC_POINT* pub_point = EC_POINT_new(EC_KEY_get0_group(ec_key));
    if (!pub_point)
    {
        EC_KEY_free(ec_key);
        throw std::runtime_error("Failed to allocate EC_POINT for public key");
    }
    if (!EC_POINT_oct2point(EC_KEY_get0_group(ec_key), pub_point, pub_bytes.data(), pub_bytes.size(), nullptr)) {
        std::ostringstream oss;
        oss << "Failed to convert public key bytes to EC_POINT. Bytes: " << publicKey.toHex();
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        throw std::runtime_error(oss.str());
    }
    if (!EC_KEY_set_public_key(ec_key, pub_point)) {
        std::ostringstream oss;
        oss << "Failed to set EC public key. EC_POINT may be invalid. Bytes: " << publicKey.toHex();
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        throw std::runtime_error(oss.str());
    }
    EC_POINT_free(pub_point);
    // Use only r+s for verification (v is not used by OpenSSL)
    const auto& sig_bytes = signature.bytes();
    int verify_status = ECDSA_verify(0, hash.data(), hash.size(), sig_bytes.data(), 64, ec_key);
    EC_KEY_free(ec_key);
    if (verify_status != 1) {
        std::ostringstream oss;
        oss << "ECDSA_verify failed. Signature: " << signature.toHex() << ", Hash: " << bytesToHex(hash.data(), hash.size());
        throw std::runtime_error(oss.str());
    }
    return true;
}
