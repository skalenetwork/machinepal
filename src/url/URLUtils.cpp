#include "common.h"
#include "URLUtils.h"
#include <boost/url/url.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/error.hpp>

using namespace boost::urls;

std::string URLUtils::getLocationFromUrl(const std::string& urlStr) {
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

bool URLUtils::isIpAddress(const std::string& host) {
    // Try IPv4 first
    boost::system::result<boost::urls::ipv4_address> v4 = boost::urls::parse_ipv4_address(host);
    if (v4)
        return true;

    // Try IPv6 (with or without brackets)
    std::string cleanHost = host;
    if (!cleanHost.empty() && cleanHost.front() == '[' && cleanHost.back() == ']')
        cleanHost = cleanHost.substr(1, cleanHost.size() - 2);

    boost::system::result<boost::urls::ipv6_address> v6 = boost::urls::parse_ipv6_address(cleanHost);
    return v6.has_value();
}

bool URLUtils::isDomainName(const std::string& host) {
    using namespace boost::urls;
    using boost::system::result;

    // Wrap host in dummy authority so Boost can parse it
    // e.g., "example.com" → "example.com:80"
    result<authority_view> res = parse_authority(host);

    if (!res) {
        // Not even a valid authority syntax
        return false;
    }

    authority_view auth = *res;
    host_type ht = auth.host_type();

    return ht == host_type::name;
}