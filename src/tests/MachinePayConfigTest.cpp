#include "../config/ConfigLoader.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <map>

#include "MachinePayApp.h"
#include "config/ConfigManager.h"
#include "nlohmann/json.hpp"

BOOST_AUTO_TEST_CASE(deserialize_basic_proxy_config) {
    // Path to the test config file
    std::map<std::string, std::string> configMap = {
        {"CONFIG", "src/tests/configs/basic/machinepay.yml"}
    };
    auto app = MachinePayApp::makeInstance(configMap);


    auto config = app->configManager()->latestConfig();

    BOOST_TEST(config->server()->http());// Check frontend
    BOOST_TEST(config->server()->http()->isEnabled() == true);
    BOOST_TEST(config->server()->https()->isEnabled() == true);
    BOOST_TEST(config->server()->http()->port() == 8080);
    BOOST_TEST(config->server()->https()->port() == 8443);


    BOOST_TEST(config->server()->https()->certFile().string().ends_with("certs/insecure_test_localhost.crt"));
    BOOST_TEST(config->server()->https()->keyFile().string().ends_with("secrets/insecure_test_localhost.key"));
//  BOOST_TEST(config->server()->https()->keyPassFile() == "secrets/key_password");
//    BOOST_TEST(config->server()->https()->caFile().has_value());
//    BOOST_TEST(config->server()->https()->caFile().value() == "certs/ca.crt");

    // Check facilitator
    BOOST_TEST(config->facilitator()->type() == "cdp");
    BOOST_TEST(config->facilitator()->baseUrl() == "https://api.coinbase.com/v2");
    BOOST_TEST(config->facilitator()->apiKeyFile().has_value());
    BOOST_TEST(config->facilitator()->apiKeyFile().value().string().ends_with("secrets/coinbase_api_key.txt"));
}
