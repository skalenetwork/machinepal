//
// Created by kladko on 11/17/25.
//

#include "TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

std::pair<std::string, std::string> TLSCertGenerator::generateSelfSignedCert(
    const std::string& commonName,
    const std::string& organization,
    const std::string& country,
    int validDays ) {

    // New RSA key generation using EVP_PKEY_CTX (OpenSSL 3.0 compliant)
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) return {};
    if (EVP_PKEY_keygen_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { EVP_PKEY_CTX_free(ctx); return {}; }
    EVP_PKEY_CTX_free(ctx);

    X509* x509 = X509_new();
    if (!x509) { EVP_PKEY_free(pkey); return {}; }

    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * validDays);
    X509_set_pubkey(x509, pkey);

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

    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // Write certificate to memory BIO
    BIO* certBio = BIO_new(BIO_s_mem());
    if (!certBio) { X509_free(x509); EVP_PKEY_free(pkey); return {}; }
    if (PEM_write_bio_X509(certBio, x509) != 1) {
        BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // Write private key (PKCS#8) to memory BIO
    BIO* keyBio = BIO_new(BIO_s_mem());
    if (!keyBio) { BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {}; }
    if (PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(keyBio); BIO_free(certBio); X509_free(x509); EVP_PKEY_free(pkey); return {};
    }

    // Extract certificate PEM
    char* certData = nullptr;
    long certLen = BIO_get_mem_data(certBio, &certData);
    std::string certPem;
    if (certLen > 0 && certData) certPem.assign(certData, static_cast<size_t>(certLen));

    // Extract key PEM
    char* keyData = nullptr;
    long keyLen = BIO_get_mem_data(keyBio, &keyData);
    std::string keyPem;
    if (keyLen > 0 && keyData) keyPem.assign(keyData, static_cast<size_t>(keyLen));

    BIO_free(certBio);
    BIO_free(keyBio);
    X509_free(x509);
    EVP_PKEY_free(pkey);

    return {certPem, keyPem};
}
