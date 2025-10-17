#include "OrganizationConfig.h"

ptr<OrganizationConfig> OrganizationConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager) {
    CHECK_STATE(fileManager);
    std::string organizationName = j.value("name", "");
    auto resources = ResourceConfig::createVectorFromJsonArray(j, fileManager);
    return ptr<OrganizationConfig>(new OrganizationConfig(resources, organizationName));
}

std::shared_ptr<std::vector<ptr<OrganizationConfig>>> OrganizationConfig::createVectorFromJsonArray(const nlohmann::json& j, ptr<FileManager> fileManager) {
    try {
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
    } catch (const std::exception& ex) {
        RETHROW_NESTED;
    }
}

ptr<OrganizationConfig> OrganizationConfig::createDefaultFromResources(ptr<vector<ptr<ResourceConfig>>> resources) {
    return ptr<OrganizationConfig>(new OrganizationConfig(resources, ""));
}
