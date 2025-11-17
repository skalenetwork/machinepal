#include <boost/test/unit_test.hpp>
#include "admin/TLSCertGenerator.h"
#include <openssl/x509.h>
#include <openssl/pem.h>

BOOST_AUTO_TEST_CASE(SelfSignedCertBasic) {
    TLSCertGenerator gen;
    auto pem = gen.generateSelfSignedCert("localhost", "Localhost Dev", "US", 30);

    BOOST_TEST(!pem.empty());

    BOOST_TEST(pem.find("-----BEGIN CERTIFICATE-----") == 0);

    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    BOOST_TEST(bio != nullptr);

    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BOOST_TEST(cert != nullptr);

    X509_NAME* subj = X509_get_subject_name(cert);
    BOOST_TEST(subj != nullptr);

    int idx = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
    BOOST_TEST(idx >= 0);

    X509_NAME_ENTRY* cnEntry = X509_NAME_get_entry(subj, idx);
    BOOST_TEST(cnEntry != nullptr);

    ASN1_STRING* cnAsn1 = X509_NAME_ENTRY_get_data(cnEntry);
    BOOST_TEST(cnAsn1 != nullptr);

    std::string cn(reinterpret_cast<const char*>(ASN1_STRING_get0_data(cnAsn1)),
                   ASN1_STRING_length(cnAsn1));
    BOOST_TEST(cn == "localhost");

    X509_free(cert);
    BIO_free(bio);
}
