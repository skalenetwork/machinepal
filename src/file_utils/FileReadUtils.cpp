//
// Created by stan on 08/10/25.
//
#include "common.h"
#include "FileReadUtils.h"
#include <filesystem>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>

void FileReadUtils::checkFileExistsAndReadable(const std::string& path)
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

void FileReadUtils::checkPEMFormat(const std::string& certPath, const std::string& keyPath) {
    namespace fs = std::filesystem;
    auto cwd = fs::current_path().string();
    FILE* certFile = fopen(certPath.c_str(), "r");
    if (!certFile) {
        throw std::runtime_error("Cannot open certificate file: " + certPath + ". Current working directory: " + cwd);
    }
    X509* cert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);
    if (!cert) {
        throw std::runtime_error("Certificate file is not a well-formed PEM: " + certPath + ". Current working directory: " + cwd);
    }
    FILE* keyFile = fopen(keyPath.c_str(), "r");
    if (!keyFile) {
        throw std::runtime_error("Cannot open key file: " + keyPath + ". Current working directory: " + cwd);
    }
    EVP_PKEY* pkey = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    fclose(keyFile);
    if (!pkey) {
        throw std::runtime_error("Key file is not a well-formed PEM: " + keyPath + ". Current working directory: " + cwd);
    }
    X509_free(cert);
    EVP_PKEY_free(pkey);
}
