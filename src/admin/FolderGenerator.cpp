#include "FolderGenerator.h"
#include "MachinePayCommon.h"
#include <stdexcept>

FolderGenerator::FolderGenerator(const boost::filesystem::path& baseDir)
    : base_(baseDir) {}

void FolderGenerator::createAndSecure(const boost::filesystem::path& p) {
    // Create directory (and parents) if missing
    if (!boost::filesystem::exists(p)) {
        boost::filesystem::create_directories(p);
    } else if (!boost::filesystem::is_directory(p)) {
        throw std::runtime_error("Path exists but is not a directory: " + p.string());
    }
    // Harden permissions: owner only
    boost::filesystem::permissions(
        p,
        boost::filesystem::owner_read |
        boost::filesystem::owner_write |
        boost::filesystem::owner_exe
    );
}

void FolderGenerator::generateFolderStructure() {
    try {
        createAndSecure(base_ / "certs");
        createAndSecure(base_ / "data");
        createAndSecure(base_ / "resources");
        createAndSecure(base_ / "secrets");
    } catch (...) {
        RETHROW_NESTED2("FolderGenerator failed to create secure directories under base: " + base_.string());
    }
}

