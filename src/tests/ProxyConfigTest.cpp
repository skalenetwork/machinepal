#include "../config/ProxyConfigLoader.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include "nlohmann/json.hpp"

BOOST_AUTO_TEST_CASE(deserialize_basic_proxy_config) {
    // Path to the test config file
    std::string configPath = "src/tests/configs/basic/.proxyConfig.yml";
    std::string schemaPath = ""; // Provide schema path if needed, or leave empty if not used in test

    // Load config
    ProxyConfig config = ProxyConfigLoader::load(configPath, schemaPath);

    // Check frontend
    BOOST_TEST(config.frontEnd.httpEnabled == true);
    BOOST_TEST(config.frontEnd.httpsEnabled == false);
    BOOST_TEST(config.frontEnd.httpPort == 8080);
    BOOST_TEST(config.frontEnd.httpsPort == 8443);
    BOOST_TEST(config.frontEnd.tls.certFile == "certs/proxy.crt");
    BOOST_TEST(config.frontEnd.tls.keyFile == "certs/proxy.key");
    BOOST_TEST(config.frontEnd.tls.keyPassFile == "secrets/key_password");
    BOOST_TEST(config.frontEnd.tls.caFile.has_value());
    BOOST_TEST(config.frontEnd.tls.caFile.value() == "certs/ca.crt");

    // Check facilitator
    BOOST_TEST(config.facilitator.type == "cdp");
    BOOST_TEST(config.facilitator.baseUrl == "https://api.coinbase.com/v2");
    BOOST_TEST(config.facilitator.apiKeyFile.has_value());
    BOOST_TEST(config.facilitator.apiKeyFile.value() == "secrets/coinbase_api_key");
}
