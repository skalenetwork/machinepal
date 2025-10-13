#include "OrganizationConfig.h"

ptr<OrganizationConfig> OrganizationConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    auto server = ServerConfig::createFromJson(j["server"], fileManager);
    std::string organizationName = j.value("name", "");
    return std::make_shared<OrganizationConfig>(server, organizationName);
}

