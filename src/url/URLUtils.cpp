#include "MachinePayCommon.h"
#include "URLUtils.h"

#include <boost/locale/encoding.hpp>
#include <boost/url/decode_view.hpp>
#include <boost/url/url.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/error.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <cstring>

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

bool URLUtils::decodePath(const std::string &path, std::string& result, std::string &errorMessage) {
    try {
        if (path.empty()) {
            errorMessage = "Empty URL path " + path;
            goto error;
        }


        std::string decodedPath;
        try {
            auto decoded = boost::urls::decode_view(path);
            decodedPath = std::string(decoded.begin(), decoded.end());
        } catch (const std::exception &e) {
            errorMessage = "Path contains invalid characters";
            goto error;;
        }
        if (decodedPath.empty()) {
            errorMessage = "Empty decoded URL path " + path;
            goto error;
        }
        if (decodedPath.front() != '/') {
            errorMessage = "URL path does not start with '/'";
            goto error;;;
        }
        // Reject traversal attempts (including encoded)
        if (decodedPath.find("..") != std::string::npos) {
            errorMessage = "URL path traversal not allowed";
            goto error;;
        }


        std::wstring wideText = boost::locale::conv::to_utf<wchar_t>(decodedPath, "UTF-8");

        for (wchar_t ch: wideText) {
            auto isValid = iswalnum(ch) || ch == L'/';
            if (!isValid) {
                errorMessage = "URL path contains invalid character:" + path;
                goto error;
            }
        }

        result = decodedPath;
        return true;
    } catch (std::exception &e) {
        errorMessage = e.what();
        goto error;
    }

    error:
        spdlog::error("Error parsing user submitted URL path in X402Processor: {}", errorMessage);
    return false;
}

std::string URLUtils::base64Encode(const std::string& input) {
    std::string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(input.size()));
    boost::beast::detail::base64::encode(&encoded[0], input.data(), input.size());
    encoded.resize(strnlen(encoded.c_str(), encoded.size()));
    return encoded;
}
