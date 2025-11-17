//
// Created by kladko on 11/17/25.
//

#ifndef MACHINEPAY_TLSCERTGENERATOR_H
#define MACHINEPAY_TLSCERTGENERATOR_H

#include <string>


class TLSCertGenerator {
public:
    std::string generateSelfSignedCert(
        const std::string& commonName,
        const std::string& organization,
        const std::string& country,
        int validDays );
};


#endif //MACHINEPAY_TLSCERTGENERATOR_H