#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "MachinePayCommon.h"
#include "EthPrivateKey.h"
#include "Keccak.h"
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cctype>
#include <cstring>
#include <openssl/bn.h>
#include <string>

static std::string trimCopy(const std::string& in) {
    size_t start = 0; while (start < in.size() && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
    size_t end = in.size(); while (end > start && std::isspace(static_cast<unsigned char>(in[end-1]))) --end;
    return in.substr(start, end - start);
}


static std::vector<uint8_t> hexToBytesFlexible(const std::string& hex) {
    std::string s = hex;

    // Strip optional 0x or 0X prefix
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        s = s.substr(2);

    // Must have even length
    if (s.size() % 2 != 0)
        throw std::invalid_argument("Hex string must have even length");

    // Optional strict length check (for Ethereum private key = 32 bytes)
    if (s.size() != 64)
        throw std::invalid_argument("Private key must be 32 bytes (64 hex chars); got " + std::to_string(s.size()));

    std::vector<uint8_t> bytes;
    bytes.reserve(s.size() / 2);

    try {
        boost::algorithm::unhex(s.begin(), s.end(), std::back_inserter(bytes));
    } catch (const boost::algorithm::hex_decode_error& e) {
        throw std::invalid_argument(std::string("Invalid hex input: ") + e.what());
    }

    return bytes;
}


EthPrivateKey::EthPrivateKey() : bytes_{} {}

EthPrivateKey::EthPrivateKey(const std::array<uint8_t, 32>& bytes) : bytes_(bytes) {}

EthPrivateKey::EthPrivateKey(std::span<const uint8_t, 32> bytes) {
    std::copy(bytes.begin(), bytes.end(), bytes_.begin());
}

EthPrivateKey::EthPrivateKey(const uint8_t* data, std::size_t len) {
    if (len != 32) {
        throw std::invalid_argument("EthPrivateKey must be 32 bytes");
    }
    std::copy(data, data + len, bytes_.begin());
}

EthPrivateKey::EthPrivateKey(const std::string& hex) {
    auto v = hexToBytesFlexible(hex);
    std::copy(v.begin(), v.end(), bytes_.begin());
    if (!isValidRange(bytes_)) throw std::invalid_argument("Private key out of range for secp256k1");
}

EthPrivateKey::~EthPrivateKey() {
    volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(bytes_.data());
    for (size_t i=0;i<bytes_.size();++i) p[i]=0;
}

bool EthPrivateKey::isValidRange(const std::array<uint8_t,32>& k) {
    BIGNUM* bn = BN_bin2bn(k.data(), 32, nullptr);
    if (!bn) return false;
    BIGNUM* n = nullptr;
    BN_hex2bn(&n, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
    bool ok = !BN_is_zero(bn) && BN_cmp(bn, n) < 0; // 0 < k < n
    BN_free(bn); BN_free(n);
    return ok;
}

EthPrivateKey EthPrivateKey::parseFlexible(const std::string& hex) {
    auto v = hexToBytesFlexible(hex);
    std::array<uint8_t,32> arr{}; std::copy(v.begin(), v.end(), arr.begin());
    // Check that private key is not zero
    if (std::all_of(arr.begin(), arr.end(), [](uint8_t b){ return b == 0; })) {
        throw std::invalid_argument("Private key must not be zero");
    }
    if (!isValidRange(arr)) throw std::invalid_argument("Private key out of range for secp256k1");
    return EthPrivateKey(arr);
}

EthPrivateKey EthPrivateKey::parseHex(const std::string& hex) { return parseFlexible(hex); }

std::string EthPrivateKey::toHex() const {
    std::string out;
    out.reserve(66); // 2 for '0x' + 64 for 32 bytes
    out += "0x";
    boost::algorithm::hex_lower(bytes_.begin(), bytes_.end(), std::back_inserter(out));
    return out;
}

bool operator==(const EthPrivateKey& a, const EthPrivateKey& b) { return a.bytes_ == b.bytes_; }
bool operator!=(const EthPrivateKey& a, const EthPrivateKey& b) { return !(a == b); }


EthPrivateKey EthPrivateKey::generateHardHatCompatibleEthereumPrivateKey() {
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
    return privateKey;
}

EthPublicKey EthPrivateKey::derivePublicKeyFromPrivateKey(const EthPrivateKey &key) {
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

#pragma GCC diagnostic pop
