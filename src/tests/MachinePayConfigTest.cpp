#include "../config/MachinePayConfigLoader.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include "nlohmann/json.hpp"

BOOST_AUTO_TEST_CASE(deserialize_basic_proxy_config) {
    // Path to the test config file
    std::string configPath = "src/tests/configs/basic/machinepay.yml";
    std::string schemaPath; // Provide schema path if needed, or leave empty if not used in test

    // Load config
    MachinePayConfig config = MachinePayConfigLoader::load(configPath, schemaPath);

    // Check frontend
    BOOST_TEST(config.server().httpEnabled() == true);
    BOOST_TEST(config.server().httpsEnabled() == false);
    BOOST_TEST(config.server().httpPort().value() == 8080);
    BOOST_TEST(config.server().httpsPort().value() == 8443);

    BOOST_TEST(config.server().tls().has_value());


    BOOST_TEST(config.server().tls().value().certFile() == "certs/machinepay.crt");
    BOOST_TEST(config.server().tls().value().keyFile() == "certs/machinepay.key");
    BOOST_TEST(config.server().tls().value().keyPassFile() == "secrets/key_password");
    BOOST_TEST(config.server().tls().value().caFile().has_value());
    BOOST_TEST(config.server().tls().value().caFile().value() == "certs/ca.crt");

    // Check facilitator
    BOOST_TEST(config.facilitator().type() == "cdp");
    BOOST_TEST(config.facilitator().baseUrl() == "https://api.coinbase.com/v2");
    BOOST_TEST(config.facilitator().apiKeyFile().has_value());
    BOOST_TEST(config.facilitator().apiKeyFile().value() == "secrets/coinbase_api_key");
}
