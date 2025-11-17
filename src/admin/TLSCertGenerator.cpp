//
// Created by kladko on 11/17/25.
//

#include "TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

#include "TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bn.h>

std::pair<std::string, std::string> TLSCertGenerator::generateSelfSignedCert(
    const std::string& commonName,
    const std::string& organization,
    const std::string& country,
    int validDays ) {

    // 1. Key Generation (same as before)
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) return {};
    if (EVP_PKEY_keygen_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    EVP_PKEY_CTX_free(ctx);

    // 2. Create X509 Certificate Structure
    X509* x509 = X509_new();
    if (!x509) { EVP_PKEY_free(pkey); return {}; }

    X509_set_version(x509, 2); // v3 certificate

    // --- NEW: Create a random serial number ---
    BIGNUM *bignum = BN_new();
    if (!bignum || !BN_rand(bignum, 64, -1, 0)) {
        X509_free(x509); EVP_PKEY_free(pkey); if(bignum) BN_free(bignum); return {};
    }
    BN_to_ASN1_INTEGER(bignum, X509_get_serialNumber(x509));
    BN_free(bignum);
    // --- End NEW ---

    // Set validity
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * validDays);
    X509_set_pubkey(x509, pkey);

    // 3. Set Subject and Issuer Name
    X509_NAME* name = X509_get_subject_name(x509);
    if (!name) { X509_free(x509); EVP_PKEY_free(pkey); return {}; }

    if (!country.empty())
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>(country.c_str()),
                                   -1, -1, 0);
    if (!organization.empty())
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>(organization.c_str()),
                                   -1, -1, 0);
    if (!commonName.empty())
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>(commonName.c_str()),
                                   -1, -1, 0);

    X509_set_issuer_name(x509, name);

    // --- NEW: Add X.509 v3 Extensions ---
    // This helper function adds extensions. We need to define it or inline it.
    // Let's inline the logic for simplicity.
    auto add_ext = [&](int nid, const char *value) {
        X509_EXTENSION *ex;
        X509V3_CTX ctx;
        X509V3_set_ctx_nodb(&ctx);
        X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0); // Self-signed
        ex = X509V3_EXT_conf_nid(nullptr, &ctx, nid, value);
        if (!ex) return false;
        if (X509_add_ext(x509, ex, -1) == 0) {
            X509_EXTENSION_free(ex);
            return false;
        }
        X509_EXTENSION_free(ex); // X509_add_ext makes an internal copy
        return true;
    };

    // 1. Basic Constraints (Mark as a CA)
    if (!add_ext(NID_basic_constraints, "critical,CA:TRUE")) {
        X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // 2. Key Usage
    if (!add_ext(NID_key_usage, "critical,digitalSignature,keyEncipherment")) {
        X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // 3. Subject Alternative Name (CRITICAL)
    std::string san_str = "DNS:" + commonName;
    if (!add_ext(NID_subject_alt_name, san_str.c_str())) {
        X509_free(x509); EVP_PKEY_free(pkey); return {};
    }
    // --- End NEW ---


    // 4. Sign the certificate
    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // 5. Write certificate to memory BIO (same as before)
    BIO* certBio = BIO_new(BIO_s_mem());
    if (!certBio) { X509_free(x509); EVP_PKEY_free(pkey); return {}; }
    if (PEM_write_bio_X509(certBio, x509) != 1) {
        BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // 6. Write private key to memory BIO (same as before)
    BIO* keyBio = BIO_new(BIO_s_mem());
    if (!keyBio) { BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {}; }
    if (PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(keyBio); BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // 7. Extract PEM strings (same as before)
    char* certData = nullptr;
    long certLen = BIO_get_mem_data(certBio, &certData);
    std::string certPem;
    if (certLen > 0 && certData) certPem.assign(certData, static_cast<size_t>(certLen));

    char* keyData = nullptr;
    long keyLen = BIO_get_mem_data(keyBio, &keyData);
    std::string keyPem;
    if (keyLen > 0 && keyData) keyPem.assign(keyData, static_cast<size_t>(keyLen));

    // 8. Cleanup (same as before)
    BIO_free(certBio);
    BIO_free(keyBio);
    X509_free(x509);
    EVP_PKEY_free(pkey);

    return {certPem, keyPem};
}
