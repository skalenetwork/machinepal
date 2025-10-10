#include "HTTPSConfig.h"
#include "crypto/CertManager.h"
#include <stdexcept>

HTTPSConfig::HTTPSConfig(bool isEnabled, uint16_t port, const CanonicalPath& certFile,
                         const CanonicalPath& keyFile,
                         const std::optional<CanonicalPath> keyPassFile,
                         const std::optional<CanonicalPath>& caFile)
    : HTTPConfig(isEnabled, port), certFile_(certFile), keyFile_(keyFile), keyPassFile_(keyPassFile), caFile_(caFile)
{
    CHECK_STATE(port_ > 0);
    CertManager::doThoroughKeyCertFormatCheck(certFile_, keyFile_);
}

const CanonicalPath& HTTPSConfig::certFile() const { return certFile_; }
const CanonicalPath& HTTPSConfig::keyFile() const { return keyFile_; }
const std::optional<CanonicalPath>& HTTPSConfig::keyPassFile() const { return keyPassFile_; }
const std::optional<CanonicalPath>& HTTPSConfig::caFile() const { return caFile_; }

