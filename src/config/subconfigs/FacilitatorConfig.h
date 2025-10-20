#pragma once
#include "common.h"
#include "filesystem/CanonicalPath.h"

class FileManager;
class CanonicalPath;

enum class FacilitatorType {
    cdp,
    base,
};


class FacilitatorConfig {
    FacilitatorType type_;
    std::string baseUrl_;
    std::optional<CanonicalPath> apiKeyFile_;

    FacilitatorConfig(const FacilitatorType,
                      const std::string& baseUrl,
                      const std::optional<CanonicalPath>& apiKeyFile = std::nullopt);
public:
    const FacilitatorType type() const;
    const std::string& baseUrl() const;
    const std::optional<CanonicalPath>& apiKeyFile() const;
    static ptr<FacilitatorConfig> createFomJson(const nlohmann::json& j, ptr<FileManager> fileManager);



    static FacilitatorType mustContainType(const nlohmann::json& j);


};

inline std::string to_string(FacilitatorType type) {
    switch (type) {
        case FacilitatorType::cdp: return "cdp";
        case FacilitatorType::base: return "base";
        throw std::invalid_argument("Unknown FacilitatorType");
    }
}
