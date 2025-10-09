#pragma once

#include <filesystem>
#include <string>

class FileManager
{
public:
    explicit FileManager(const std::string &userProvidedCondigPath)
        : userProvidedConfigPath_(userProvidedCondigPath) {
        checkFileExistsAndReadable(userProvidedCondigPath);
        canonicalConfigPath_ = resolveCanonicalPathAgainstCwd(userProvidedConfigPath_);
    }

    static void checkFileExistsAndReadable(const std::string& path);

    static std::chrono::system_clock::time_point getLastFileModificationTime(string& _path);

    static filesystem::path resolveCanonicalPathAgainstCwd(std::string _path);

private:
    std::string userProvidedConfigPath_;
    std::string canonicalConfigPath_;
};
