#include "URLUtils.h"
#include "MachinePayCommon.h"

using namespace boost::urls;

std::string URLUtils::getLocationFromUrl(const std::string &urlStr) {
    // Use parse_uri_reference to handle "/path" and "http://host/path"
    auto result = boost::urls::parse_uri_reference(urlStr);

    if (!result) {
        spdlog::error("Invalid URL provided: {}", urlStr);
        throw std::invalid_argument("Invalid URL: " + urlStr);
    }

    boost::urls::url_view u = *result;

    // Use buffer to build path + query safely
    std::string path;
    if (u.encoded_path().empty()) {
        path = "/";
    } else {
        path = std::string(u.encoded_path());
    }

    if (!u.encoded_query().empty()) {
        path += "?";
        path += u.encoded_query();
    }

    return path;
}

bool URLUtils::isIpAddress(const std::string &host) {
    using namespace boost::urls;

    // Trim whitespace using Boost
    std::string_view h = boost::algorithm::trim_copy(host);

    // Try IPv4
    if (parse_ipv4_address(h).has_value())
        return true;

    // Handle bracketed IPv6: [::1]
    if (h.size() >= 2 && h.front() == '[' && h.back() == ']')
        h = h.substr(1, h.size() - 2);

    // Try IPv6
    return parse_ipv6_address(h).has_value();
}

bool URLUtils::isDomainName(const std::string &host) {
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

bool URLUtils::isValidUrl(const std::string &url) {
    // parse_uri returns a result object which is true on success
    // and false on failure.
    boost::system::result<boost::urls::url_view> result = boost::urls::parse_uri(url);
    return result.has_value();
}


bool URLUtils::decodePath(const std::string& path, std::string& result, std::string& errorMessage) {
    if (path.empty() || path[0] != '/') {
        errorMessage = "Path must start with /";
        return false;
    }

    // 1. Parse as a URI reference.
    // This validates the %-encoding (e.g., rejects "%2") and structure.
    boost::system::result<boost::urls::url_view> rv = boost::urls::parse_uri_reference(path);

    if (rv.has_error()) {
        errorMessage = "Path contains invalid encoding: " + rv.error().message();
        return false;
    }

    // 2. Decode the path safely.
    // rv->encoded_path() returns a validated pct_string_view.
    // .decode() converts it to std::string.
    std::string decoded = rv->encoded_path().decode();

    // --- Security Checks (Same as before) ---

    // Check for null bytes (poisoning)
    if (decoded.find('\0') != std::string::npos) {
        errorMessage = "Null byte detected";
        return false;
    }

    // Check for ".." traversal
    if (decoded.find("/../") != std::string::npos ||
        decoded.ends_with("/..") ||
        decoded == "..") {
        errorMessage = "Path traversal attempt";
        return false;
        }

    // Character whitelist
    for (char ch : decoded) {
        bool isSafe = std::isalnum(static_cast<unsigned char>(ch)) ||
                      ch == '/' || ch == '.' || ch == '-' || ch == '_';

        if (!isSafe) {
            errorMessage = "Invalid character in path";
            return false;
        }
    }

    result = decoded;
    return true;
}