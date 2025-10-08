#pragma once

#include <string>

class FileReadUtils
{

public:

    static void checkFileExistsAndReadable(const std::string& path);
    static void checkPEMFormat(const std::string& certPath, const std::string& keyPath);
    static void checkKeyMatchesCert(const std::string& certPath, const std::string& keyPath);
    static void doThoroughKeyCertFormatCheck(const std::string& certPath, const std::string& keyPath);

    static void validateSSLContext(const std::string& certFile, const std::string& keyFile, const std::string& caFile);

};
