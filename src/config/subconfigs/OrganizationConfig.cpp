#include "OrganizationConfig.h"

#include "config/JsonUtils.h"
#include "exceptions/JsonValidationException.h"


void OrganizationConfig::validateOrgName(const std::string &name) {
    // Rule 1: Length must be between 1 and 39 characters
    if (name.empty() || name.size() > 39) {
        throw JsonValidationException(
            "Invalid organization name: " + name + ". It must be between 1 and 39 characters.",
            nlohmann::json::object());
    }
    static const std::regex pattern("^[a-z0-9]+(-[a-z0-9]+)*$");
    auto result = std::regex_match(name, pattern);
    if (!result) {
        throw JsonValidationException(
            "Invalid organization name: " + name +
            ". It must be between 1 and 39 characters, only contain lowercase letters, "
            "numbers, and hyphens, "
            "cannot start or end with a hyphen, and cannot have consecutive hyphens.",
            nlohmann::json::object());
    }
}

ptr<OrganizationConfig> OrganizationConfig::createFromJson(
    const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        std::string organizationName = JsonUtils::mustContainString("name", j);
        std::transform(
            organizationName.begin(), organizationName.end(), organizationName.begin(), ::tolower);
        validateOrgName(organizationName);
        std::string subdomain = JsonUtils::mustContainString("subdomain", j);
        auto resources = ResourceConfig::createVectorFromJsonArray(j, fileManager);

        bool isPassThrough = false;

        if (j.contains("isPassThrough")) {
            CHECK_STATE_JSON(j.at("isPassThrough").is_object(), "isPassThrough must be object", j);
            auto isPassThroughObj = j.at("isPassThrough");
            isPassThrough = JsonUtils::getBoolWithDefault(j.at("isPassThrough"), isPassThroughObj, false);;
        }

        auto defaultOrganization = OrganizationConfig::createDefaultFromResources( resources,
            isPassThrough);

        return ptr<OrganizationConfig>(
            new OrganizationConfig(resources, organizationName, subdomain, isPassThrough));
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}

std::shared_ptr<std::vector<ptr<OrganizationConfig> > >
OrganizationConfig::createVectorFromJsonArray(
    const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        auto result = std::make_shared<std::vector<ptr<OrganizationConfig> > >();
        if (!j.contains("organizations"))
            return result;
        auto organizations = j.at("organizations");
        CHECK_STATE(organizations.is_array());
        for (const auto &item: organizations) {
            auto org = OrganizationConfig::createFromJson(item, fileManager);
            if (org)
                result->push_back(org);
        }
        return result;
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}

ptr<OrganizationConfig> OrganizationConfig::createDefaultFromResources(
    ptr<vector<ptr<ResourceConfig> > > resources, bool isPassThrough) {
    try {
        return ptr<OrganizationConfig>(new OrganizationConfig(resources, "", "", isPassThrough));
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}
