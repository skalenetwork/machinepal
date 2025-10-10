#pragma once
#include <filesystem>
#include <optional>
#include "HTTPConfig.h"

class HTTPSConfig : public HTTPConfig {
    std::filesystem::path certFile_;
    std::filesystem::path keyFile_;
    std::optional<std::filesystem::path> keyPassFile_;
    std::optional<std::filesystem::path> caFile_;
public:
    HTTPSConfig(bool isEnabled, uint16_t port, const std::filesystem::path& certFile,
                const std::filesystem::path& keyFile,
                const std::optional<std::filesystem::path> keyPassFile,
                const std::optional<std::filesystem::path>& caFile);
    const std::filesystem::path& certFile() const;
    const std::filesystem::path& keyFile() const;
    const std::optional<std::filesystem::path>& keyPassFile() const;
    const std::optional<std::filesystem::path>& caFile() const;
};

