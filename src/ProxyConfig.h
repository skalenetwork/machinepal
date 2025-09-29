#pragma once
#include <string>
#include <glog/types.h>

struct ProxyConfig {
    bool httpEnabled;
    bool httpsEnabled;
    uint16_t httpPort;
    uint16_t httpsPort;
    std::string httpsCertFile;
    std::string httpsKeyFile;
    std::string httpsKeyPasswordFile // loaded from a secret file (or env)
    std::string coinbaseAPIURL;
    std::string coinbaseAPIKeyFile;  // loaded from a secret file (or env)
};
