#pragma once


class HTTPSConfig;

class CertManager {

public:

    static void checkPEMFormat(const std::string& certPath, const std::string& keyPath);
    static void checkKeyMatchesCert(const std::string& certPath, const std::string& keyPath);
    static void doThoroughKeyCertFormatCheck(const std::string& certPath, const std::string& keyPath);
    static void validateSSLFiles(const std::string& certFile, const std::string& keyFile, const std::string& caFile);
    static std::string getCaFilePath(const std::shared_ptr<HTTPSConfig>& https);

};
