#include "EthPrivateKey.h"
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

// Helper function to convert hex string to bytes
static std::vector<uint8_t> hexToBytesFlexible(const std::string& hex) {
    std::string s = trimCopy(hex);
    if (s.rfind("0x",0)==0 || s.rfind("0X",0)==0) s = s.substr(2);
    if (s.size() != 64) throw std::invalid_argument("Private key hex must be 64 characters (32 bytes); got " + std::to_string(s.size()));
    for (char c: s) if (!std::isxdigit(static_cast<unsigned char>(c))) throw std::invalid_argument("Invalid hex character in private key");
    std::vector<uint8_t> bytes; bytes.reserve(32);
    for (size_t i=0;i<s.size(); i+=2) {
        uint8_t hi = std::isdigit(s[i])? s[i]-'0' : (std::tolower(s[i])-'a'+10);
        uint8_t lo = std::isdigit(s[i+1])? s[i+1]-'0' : (std::tolower(s[i+1])-'a'+10);
        bytes.push_back((hi<<4)|lo);
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
    if (!isValidRange(arr)) throw std::invalid_argument("Private key out of range for secp256k1");
    return EthPrivateKey(arr);
}

EthPrivateKey EthPrivateKey::parseHex(const std::string& hex) { return parseFlexible(hex); }

std::string EthPrivateKey::toHex() const {
    std::stringstream ss; ss << "0x";
    for (auto b: bytes_) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return ss.str();
}

bool operator==(const EthPrivateKey& a, const EthPrivateKey& b) { return a.bytes_ == b.bytes_; }
bool operator!=(const EthPrivateKey& a, const EthPrivateKey& b) { return !(a == b); }
