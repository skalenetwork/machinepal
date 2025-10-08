#include "../config/MachinePayConfigLoader.h"
#include <boost/test/unit_test.hpp>
#include <string>

#include "config/MachinePayConfigManager.h"
#include "nlohmann/json.hpp"

BOOST_AUTO_TEST_CASE(deserialize_basic_proxy_config) {
    // Path to the test config file


    MachinePayConfigManager::getInstance().initManager({
    {
        "CONFIG",
        "src/tests/configs/basic/machinepay.yml"
    }
    });

    auto config = MachinePayConfigManager::getInstance().latestConfig();


    BOOST_TEST(config->server()->http());// Check frontend
    BOOST_TEST(config->server()->http()->isEnabled() == true);
    BOOST_TEST(config->server()->https()->isEnabled() == false);
    BOOST_TEST(config->server()->http()->port() == 8080);
    BOOST_TEST(config->server()->https()->port() == 8443);


    BOOST_TEST(config->server()->https()->certFile() == "certs/machinepay.crt");
    BOOST_TEST(config->server()->https()->keyFile() == "certs/machinepay.key");
//  BOOST_TEST(config->server()->https()->keyPassFile() == "secrets/key_password");
    BOOST_TEST(config->server()->https()->caFile().has_value());
    BOOST_TEST(config->server()->https()->caFile().value() == "certs/ca.crt");

    // Check facilitator
    BOOST_TEST(config->facilitator()->type() == "cdp");
    BOOST_TEST(config->facilitator()->baseUrl() == "https://api.coinbase.com/v2");
    BOOST_TEST(config->facilitator()->apiKeyFile().has_value());
    BOOST_TEST(config->facilitator()->apiKeyFile().value() == "secrets/coinbase_api_key");
}
