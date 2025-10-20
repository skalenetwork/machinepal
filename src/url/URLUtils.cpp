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