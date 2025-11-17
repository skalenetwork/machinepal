//
// Created by kladko on 11/17/25.
//

#ifndef MACHINEPAY_TLSCERTGENERATOR_H
#define MACHINEPAY_TLSCERTGENERATOR_H

#include <string>
#include <utility>
#include <boost/filesystem/path.hpp>


class TLSCertGenerator {
public:
    std::string getOpenSSLError();

    std::pair<std::string, std::string> generateSelfSignedCert(
        const std::string& commonName = "localhost",
        const std::string& organization = "",
        const std::string& country = "",
        int validDays = 3650);

    void generateDefaultCertFiles(
        const boost::filesystem::path& certFilePath,
        const boost::filesystem::path& keyFilePath);
};


#endif //MACHINEPAY_TLSCERTGENERATOR_H