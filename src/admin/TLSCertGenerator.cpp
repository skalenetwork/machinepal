//
// Created by kladko on 11/17/25.
//

#include "TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <memory>       // For std::unique_ptr
#include <stdexcept>    // For std::runtime_error
#include <sstream>      // For formatting error messages
#include <openssl/err.h>  // For getOpenSSLError
#include "TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bn.h>


// Helper function to get the last OpenSSL error string
std::string TLSCertGenerator::getOpenSSLError() {
    unsigned long errCode = ERR_get_error();
    if (errCode == 0) return "No OpenSSL error";
    char err_buf[256];
    ERR_error_string_n(errCode, err_buf, sizeof(err_buf));
    return std::string(err_buf);
}

// RAII wrappers for OpenSSL objects
using UniquePKEYCTX = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using UniquePKEY = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using UniqueX509 = std::unique_ptr<X509, decltype(&X509_free)>;
using UniqueBIGNUM = std::unique_ptr<BIGNUM, decltype(&BN_free)>;
using UniqueBIO = std::unique_ptr<BIO, decltype(&BIO_free)>;
using UniqueX509Ext = std::unique_ptr<X509_EXTENSION, decltype(&X509_EXTENSION_free)>;


std::pair<std::string, std::string> TLSCertGenerator::generateSelfSignedCert(
    const std::string &commonName,
    const std::string &organization,
    const std::string &country,
    int validDays) {
    // 1. Key Generation
    EVP_PKEY *pkey_raw = nullptr;
    UniquePKEYCTX ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) {
        throw std::runtime_error("EVP_PKEY_CTX_new_id failed: " + getOpenSSLError());
    }
    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
        throw std::runtime_error("EVP_PKEY_keygen_init failed: " + getOpenSSLError());
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0) {
        throw std::runtime_error("EVP_PKEY_CTX_set_rsa_keygen_bits failed: " + getOpenSSLError());
    }
    if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) <= 0) {
        throw std::runtime_error("EVP_PKEY_keygen failed: " + getOpenSSLError());
    }
    UniquePKEY pkey(pkey_raw, &EVP_PKEY_free); // Wrap raw pointer immediately

    // 2. Create X509 Certificate Structure
    UniqueX509 x509(X509_new(), &X509_free);
    if (!x509) {
        throw std::runtime_error("X509_new failed: " + getOpenSSLError());
    }

    X509_set_version(x509.get(), 2); // v3 certificate

    // Create a random serial number
    UniqueBIGNUM bignum(BN_new(), &BN_free);
    if (!bignum || !BN_rand(bignum.get(), 64, -1, 0)) {
        throw std::runtime_error("BN_rand (serial) failed: " + getOpenSSLError());
    }
    if (!BN_to_ASN1_INTEGER(bignum.get(), X509_get_serialNumber(x509.get()))) {
        throw std::runtime_error("BN_to_ASN1_INTEGER failed: " + getOpenSSLError());
    }

    // Set validity
    X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
    X509_gmtime_adj(X509_get_notAfter(x509.get()), 60L * 60L * 24L * validDays);

    if (X509_set_pubkey(x509.get(), pkey.get()) <= 0) {
        throw std::runtime_error("X509_set_pubkey failed: " + getOpenSSLError());
    }

    // 3. Set Subject and Issuer Name
    X509_NAME *name = X509_get_subject_name(x509.get());
    if (!name) {
        throw std::runtime_error("X509_get_subject_name failed");
    }

    if (!country.empty())
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(country.c_str()),
                                   -1, -1, 0);
    if (!organization.empty())
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(organization.c_str()),
                                   -1, -1, 0);
    if (!commonName.empty())
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(commonName.c_str()),
                                   -1, -1, 0);

    if (X509_set_issuer_name(x509.get(), name) <= 0) {
        throw std::runtime_error("X509_set_issuer_name failed: " + getOpenSSLError());
    }

    // --- Add X.509 v3 Extensions ---
    auto add_ext = [&](int nid, const char *value) {
        X509V3_CTX ctx;
        X509V3_set_ctx_nodb(&ctx);
        X509V3_set_ctx(&ctx, x509.get(), x509.get(), nullptr, nullptr, 0); // Self-signed

        // ex is automatically cleaned up by unique_ptr
        UniqueX509Ext ex(X509V3_EXT_conf_nid(nullptr, &ctx, nid, value), &X509_EXTENSION_free);
        if (!ex) {
            return false;
        }
        if (X509_add_ext(x509.get(), ex.get(), -1) == 0) {
            return false;
        }
        return true;
    };

    if (!add_ext(NID_basic_constraints, "critical,CA:TRUE")) {
        throw std::runtime_error("Failed to add NID_basic_constraints: " + getOpenSSLError());
    }
    if (!add_ext(NID_key_usage, "critical,digitalSignature,keyEncipherment")) {
        throw std::runtime_error("Failed to add NID_key_usage: " + getOpenSSLError());
    }
    std::string san_str = "DNS:" + commonName;
    if (!add_ext(NID_subject_alt_name, san_str.c_str())) {
        throw std::runtime_error("Failed to add NID_subject_alt_name: " + getOpenSSLError());
    }

    // 4. Sign the certificate
    if (X509_sign(x509.get(), pkey.get(), EVP_sha256()) == 0) {
        throw std::runtime_error("X509_sign failed: " + getOpenSSLError());
    }

    // 5. Write certificate to memory BIO
    UniqueBIO certBio(BIO_new(BIO_s_mem()), &BIO_free);
    if (!certBio) {
        throw std::runtime_error("BIO_new (cert) failed: " + getOpenSSLError());
    }
    if (PEM_write_bio_X509(certBio.get(), x509.get()) != 1) {
        throw std::runtime_error("PEM_write_bio_X509 failed: " + getOpenSSLError());
    }

    // 6. Write private key to memory BIO
    UniqueBIO keyBio(BIO_new(BIO_s_mem()), &BIO_free);
    if (!keyBio) {
        throw std::runtime_error("BIO_new (key) failed: " + getOpenSSLError());
    }
    if (PEM_write_bio_PrivateKey(keyBio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        throw std::runtime_error("PEM_write_bio_PrivateKey failed: " + getOpenSSLError());
    }

    // 7. Extract PEM strings
    char *certData = nullptr;
    long certLen = BIO_get_mem_data(certBio.get(), &certData);
    std::string certPem;
    if (certLen > 0 && certData) {
        certPem.assign(certData, static_cast<size_t>(certLen));
    } else {
        throw std::runtime_error("Failed to get cert PEM data from BIO");
    }

    char *keyData = nullptr;
    long keyLen = BIO_get_mem_data(keyBio.get(), &keyData);
    std::string keyPem;
    if (keyLen > 0 && keyData) {
        keyPem.assign(keyData, static_cast<size_t>(keyLen));
    } else {
        throw std::runtime_error("Failed to get key PEM data from BIO");
    }

    // 8. Cleanup - All cleanup is now handled automatically by std::unique_ptr

    return {certPem, keyPem};
}

void TLSCertGenerator::generateDefaultCertFiles(
    const boost::filesystem::path &certFilePath,
    const boost::filesystem::path &keyFilePath) {
    auto pemPair = generateSelfSignedCert(); // uses defaults (localhost, long validity)
    if (pemPair.first.empty() || pemPair.second.empty()) {
        return; // generation failed; optionally log
    }

    if (certFilePath.has_parent_path())
        boost::filesystem::create_directories(certFilePath.parent_path());
    if (keyFilePath.has_parent_path())
        boost::filesystem::create_directories(keyFilePath.parent_path());

    std::ofstream certOut(certFilePath.string(), std::ios::out | std::ios::trunc);
    if (certOut) certOut << pemPair.first;

    std::ofstream keyOut(keyFilePath.string(), std::ios::out | std::ios::trunc);
    if (keyOut) keyOut << pemPair.second;
}
