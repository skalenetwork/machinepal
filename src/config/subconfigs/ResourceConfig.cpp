#include "ResourceConfig.h"

#include "config/JsonUtils.h"
#include "exceptions/JsonValidationException.h"
#include <boost/url/url.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/error.hpp>

class FileManager;


ResourceType ResourceConfig::mustContainType(const nlohmann::json &j) {
    auto typeString = JsonUtils::mustContainString(j, "type");
    if (typeString == "local_file") {
        return ResourceType::LocalFile;
    }
    if (typeString == "api-jsonrpc") {
        return ResourceType::ApiJsonRpc;
    }

    if (typeString == "api-rest") {
        return ResourceType::ApiRest;
    }

    throw JsonValidationException("Invalid resource type: " + typeString, j);
}

ptr<ResourceConfig> ResourceConfig::createFromJson(const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        auto name = JsonUtils::mustContainString(j, "name");
        auto type = mustContainType(j);
        auto location = JsonUtils::mustContainString(j, "location");
        auto price = JsonUtils::mustContainPrice(j, "price");
        auto token = JsonUtils::mustContainString(j, "token");
        return ptr<ResourceConfig>(new ResourceConfig(name, location, type, price, token));
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}

ptr<vector<ptr<ResourceConfig> > > ResourceConfig::createVectorFromJsonArray(
    const nlohmann::json &j, ptr<FileManager> fileManager) {
    try {
        CHECK_STATE(fileManager);
        auto result = std::make_shared<std::vector<ptr<ResourceConfig> > >();
        if (!j.contains("resources"))
            return result;
        auto resources = j.at("resources");
        CHECK_STATE_JSON(resources.is_array(), "Resources must be an array resources", j);
        for (const auto &item: resources) {
            auto res = createFromJson(item, fileManager);
            CHECK_STATE(res);
            result->push_back(res);
        }
        return result;
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}

using namespace boost::urls;

std::string ResourceConfig::getLocationFromUrl(const std::string& urlStr) {
    // Parse using Boost.URL
    boost::system::result<boost::urls::url_view> result = parse_uri(urlStr);

    CHECK_STATE2(result, "Invalid URL: " + urlStr);

    url_view u = *result;
    // Return just the path (or "/" if empty)
    std::string path = u.encoded_path().empty() ? "/" : std::string(u.encoded_path());

    // Optionally include query string
    if (!u.encoded_query().empty()) {
        path += "?";
        path += u.encoded_query();
    }

    CHECK_STATE2(path.starts_with("/"), "URL path must start with '/': " + path);
    if (path.size() > 1 && path.back() == '/') {
        path = path.substr(0, path.size() - 1);
    }

    return path;
}