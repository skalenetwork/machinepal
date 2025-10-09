#pragma once

#include <string>

class FileManager
{

public:

    static void checkFileExistsAndReadable(const std::string& path);

    static std::chrono::system_clock::time_point getLastFileModificationTime(string& _path);

    static std::string resolveCanonicalPathAgainstCwd(std::string _path);
};
