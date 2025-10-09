//
// Created by stan on 08/10/25.
//
#include "common.h"
#include "FileManager.h"
#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

void FileManager::checkFileExistsAndReadable(const std::string& path)
{
    namespace fs = std::filesystem;
    auto cwd = fs::current_path().string();
    if (path.empty())
    {
        throw std::runtime_error("File path is empty. Current working directory: " + cwd);
    }
    if (!fs::exists(path))
    {
        throw std::runtime_error("File '" + path + "' does not exist. Current working directory: " + cwd);
    }

    // Check that configFile is not a directory
    if (std::filesystem::is_directory(path)) {
        throw std::runtime_error(
            "File '" + path + "' is a directory, not a file. Current working directory: " +
            std::string(cwd));
    }

    if (!fs::is_regular_file(path))
    {
        throw std::runtime_error("File '" + path + "' is not a regular file (a directory?). Current working directory: " + cwd);
    }
    if (access(path.c_str(), R_OK) != 0)
    {
        throw std::runtime_error("File '" + path + "' is not readable. Current working directory: " + cwd);
    }
    if (fs::file_size(path) == 0)
    {
        throw std::runtime_error("File '" + path + "' is empty. Current working directory: " + cwd);
    }
}

std::chrono::system_clock::time_point FileManager::getLastFileModificationTime(string& _path) {
    auto ftime = std::filesystem::last_write_time(_path);
    return std::chrono::system_clock::time_point(
        chrono::duration_cast<std::chrono::system_clock::duration>(
            ftime.time_since_epoch()
        )
    );
}

filesystem::path FileManager::resolveCanonicalPathAgainstCwd(std::string _path) {
    try {
        return std::filesystem::weakly_canonical(std::filesystem::absolute(_path)).string();
    } catch (exception& e) {
        RETHROW_NESTED("ileManager::resolveCanonicalPathAgainstCwd failed");
    }
}
