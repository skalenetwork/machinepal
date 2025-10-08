//
// Created by stan on 08/10/25.
//

#include "FileReadUtils.h"


void FileReadUtils::checkFileExistsAndReadable(const std::string& path)
{
    if (path.empty())
    {
        throw std::runtime_error("File path is empty");
    }
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("File '" + path + "' does not exist");
    }

    if (!std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error("File '" + path + "' is not a regular file (a directory?)");
    }
    if (access(path.c_str(), R_OK) != 0)
    {
        throw std::runtime_error("File '" + path + "' is not readable");
    }
    if (std::filesystem::file_size(path) == 0)
    {
        throw std::runtime_error("File '" + path + "' is empty");
    }
}
