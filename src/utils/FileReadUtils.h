#pragma once

#include <string>

class FileReadUtils
{

public:

    static void checkFileExistsAndReadable(const std::string& path);
    static void checkPEMFormat(const std::string& certPath, const std::string& keyPath);

};
