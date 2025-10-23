#pragma once

#include <string>

namespace URLUtils {

    std::string getLocationFromUrl(const std::string& url);

    bool isIpAddress(const std::string& host);

    bool isDomainName(const std::string& host);

    bool decodePath(const std::string &path, std::string& result, std::string &errorMessage);

    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);
}
