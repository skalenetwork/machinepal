#include "OrganizationConfig.h"

ptr<OrganizationConfig> OrganizationConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    auto server = ServerConfig::createFromJson(j["server"], fileManager);
    std::string organizationName = j.value("name", "");
    return std::make_shared<OrganizationConfig>(nullptr, organizationName);
}

std::shared_ptr<std::vector<ptr<OrganizationConfig>>> OrganizationConfig::createOrganizationsFromJsonArray(const nlohmann::json& j, ptr<FileManager> fileManager) {
    auto result = std::make_shared<std::vector<ptr<OrganizationConfig>>>();
    if (!j.contains("organizations"))
        return result;
    auto organizations = j.at("organizations");
    CHECK_STATE(organizations.is_array());
    for (const auto& item : organizations) {
        auto org = OrganizationConfig::createFromJson(item, fileManager);
        if (org) result->push_back(org);
    }
    return result;
}
