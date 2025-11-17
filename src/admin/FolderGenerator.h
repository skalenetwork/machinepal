#pragma once
#include <boost/filesystem/path.hpp>

class FolderGenerator {
public:
    explicit FolderGenerator(const boost::filesystem::path& baseDir);
    void generateFolderStructure(); // create certs, data, resources, secrets with secure perms
private:
    boost::filesystem::path base_;
    void createAndSecure(const boost::filesystem::path& p);
};

