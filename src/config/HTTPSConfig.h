#pragma once
#include <filesystem>
#include <optional>
#include "HTTPConfig.h"
#include "filesystem/CanonicalPath.h"

class HTTPSConfig : public HTTPConfig {
    CanonicalPath certFile_;
    CanonicalPath keyFile_;
    std::optional<CanonicalPath> keyPassFile_;
    std::optional<CanonicalPath> caFile_;
public:
    HTTPSConfig(bool isEnabled, uint16_t port, const CanonicalPath& certFile,
                const CanonicalPath& keyFile,
                const std::optional<CanonicalPath> keyPassFile,
                const std::optional<CanonicalPath>& caFile);
    const CanonicalPath& certFile() const;
    const CanonicalPath& keyFile() const;
    const std::optional<CanonicalPath>& keyPassFile() const;
    const std::optional<CanonicalPath>& caFile() const;
};
