//
// Created by stan on 08/10/25.
//
#include "common.h"
#include "FileReadUtils.h"
#include <filesystem>

void FileReadUtils::checkFileExistsAndReadable(const std::string& path)
{
    namespace fs = std::filesystem;
    if (path.empty())
    {
        throw std::runtime_error("File path is empty");
    }
    if (!fs::exists(path))
    {
        throw std::runtime_error("File '" + path + "' does not exist");
    }
    if (!fs::is_regular_file(path))
    {
        throw std::runtime_error("File '" + path + "' is not a regular file (a directory?)");
    }
    if (access(path.c_str(), R_OK) != 0)
    {
        throw std::runtime_error("File '" + path + "' is not readable");
    }
    if (fs::file_size(path) == 0)
    {
        throw std::runtime_error("File '" + path + "' is empty");
    }
}
