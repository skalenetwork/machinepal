#pragma once

#include <string>

class URLUtils {

public:

    static std::string getLocationFromUrl(const std::string& url);

    static bool isIpAddress(const std::string& host);

    static bool isDomainName(const std::string& host);

    static bool decodePath(const std::string &path, std::string& result, std::string &errorMessage);

    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
};
