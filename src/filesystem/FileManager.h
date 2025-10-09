#pragma once

#include <filesystem>
#include <string>
#include <chrono>

class FileManager
{
public:
    explicit FileManager(const std::string &userProvidedConfigPath)
        : userProvidedConfigPath_(userProvidedConfigPath) {
        checkFileExistsAndReadableCwd(userProvidedConfigPath);
        canonicalConfigPath_ = resolveCanonicalPathAgainstCwd(userProvidedConfigPath_);
    }

    filesystem::path checkFileExistsAndReadableAndResolve(const std::string& path);

    void checkFileExistsAndReadableCwd(const std::string& path);

    static std::chrono::system_clock::time_point getLastFileModificationTime(const std::string& _path);

    static std::filesystem::path resolveCanonicalPathAgainstCwd(const std::string& _path);

    std::filesystem::path resolveCanonicalPath(const std::string& _path) const;

private:
    std::string userProvidedConfigPath_;
    std::filesystem::path canonicalConfigPath_;
};
