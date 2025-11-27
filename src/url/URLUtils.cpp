#include "URLUtils.h"
#include "MachinePayCommon.h"

using namespace boost::urls;

std::string URLUtils::getLocationFromUrl(const std::string& urlStr) {
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

bool URLUtils::isIpAddress(const std::string& host) {
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

bool URLUtils::isDomainName( const std::string& host ) {
    using namespace boost::urls;
    using boost::system::result;

    // Wrap host in dummy authority so Boost can parse it
    // e.g., "example.com" → "example.com:80"
    result< authority_view > res = parse_authority( host );

    if ( !res ) {
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
bool URLUtils::decodePath(
    const std::string& path, std::string& result, std::string& errorMessage ) {
    try {
        if ( path.empty() ) {
            errorMessage = "Empty URL path " + path;
            goto error;
        }


        std::string decodedPath;
        try {
            auto decoded = boost::urls::decode_view( path );
            decodedPath = std::string( decoded.begin(), decoded.end() );
        } catch ( const std::exception& e ) {
            errorMessage = "Path contains invalid characters";
            goto error;
            ;
        }
        if ( decodedPath.empty() ) {
            errorMessage = "Empty decoded URL path " + path;
            goto error;
        }
        if ( decodedPath.front() != '/' ) {
            errorMessage = "URL path does not start with '/'";
            goto error;
            ;
            ;
        }
        // Reject traversal attempts (including encoded)
        if ( decodedPath.find( ".." ) != std::string::npos ) {
            errorMessage = "URL path traversal not allowed";
            goto error;
            ;
        }


        std::wstring wideText = boost::locale::conv::to_utf< wchar_t >( decodedPath, "UTF-8" );

        for ( wchar_t ch : wideText ) {
            auto isValid = iswalnum( ch ) || ch == L'/';
            if ( !isValid ) {
                errorMessage = "URL path contains invalid character:" + path;
                goto error;
            }
        }

        result = decodedPath;
        return true;
    } catch ( std::exception& e ) {
        errorMessage = e.what();
        goto error;
    }

error:
    spdlog::error( "Error parsing user submitted URL path in X402Processor: {}", errorMessage );
    return false;
}
