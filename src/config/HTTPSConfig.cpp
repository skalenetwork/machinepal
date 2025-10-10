#include "HTTPSConfig.h"
#include "crypto/CertManager.h"
#include <stdexcept>

HTTPSConfig::HTTPSConfig(bool isEnabled, uint16_t port, const std::filesystem::path& certFile,
                         const std::filesystem::path& keyFile,
                         const std::optional<std::filesystem::path> keyPassFile,
                         const std::optional<std::filesystem::path>& caFile)
    : HTTPConfig(isEnabled, port), certFile_(certFile), keyFile_(keyFile), keyPassFile_(keyPassFile), caFile_(caFile)
{
    CHECK_STATE(port_ > 0);
    CHECK_STATE(!certFile.empty());
    CHECK_STATE(!keyFile.empty());
    CertManager::doThoroughKeyCertFormatCheck(certFile_, keyFile_);
}

const std::filesystem::path& HTTPSConfig::certFile() const { return certFile_; }
const std::filesystem::path& HTTPSConfig::keyFile() const { return keyFile_; }
const std::optional<std::filesystem::path>& HTTPSConfig::keyPassFile() const { return keyPassFile_; }
const std::optional<std::filesystem::path>& HTTPSConfig::caFile() const { return caFile_; }

