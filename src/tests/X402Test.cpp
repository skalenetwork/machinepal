// X402HandlerBoostTest.cpp
#define BOOST_TEST_MODULE X402HandlerSelfTest

#include "MachinePayCommon.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"
#include <boost/test/included/unit_test.hpp> // or <boost/test/unit_test.hpp> if using dynamic link

#include "x402_client/X402Client.h"


#include "MachinePayApp.h"
#include "../payment/datastructures/PaymentRequirements.h"
#include "../examples/PaymentExamples.h"
#include "config/ConfigLoader.h"
#include "config/ConfigManager.h"
#include "config/subconfigs/ServerConfig.h"
#include "../payment/datastructures/PaymentRequiredResponse.h"
#include "examples/PaymentExamples.h"
#include "url/URLUtils.h"
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>


const std::string BIND_IP = "0.0.0.0";
const std::string CONNECT_HOST = "localhost";
constexpr uint32_t DEFAULT_TEST_PORT = 8080;

// ---- Global fixture that initializes glog, folly, curl -----------------------
struct X402GlobalFixture {
    X402GlobalFixture() {
        std::vector<std::string> args;
        args.push_back("x402test"); // program name
        int fake_argc = static_cast<int>(args.size());
        std::vector<char *> fake_argv;
        for (auto &s: args) {
            fake_argv.push_back(const_cast<char *>(s.c_str()));
        }
        Init::initAllLibs(fake_argc, fake_argv.data());

        // Direct log output (errors, failures) to std::cerr
        boost::unit_test::unit_test_log.set_stream(std::cerr);
    }

    ~X402GlobalFixture() {
    }
};

BOOST_GLOBAL_FIXTURE(X402GlobalFixture);


struct X402ServerFixture {
    X402ServerFixture() {
        try {
            std::map<std::string, std::string> configMap = {
                {"CONFIG", "src/tests/configs/basic/machinepay.yml"}
            };
            app_ = MachinePayApp::makeInstance(configMap);
            auto config = app_->configManager()->latestConfig();
            client = std::make_shared<X402Client>(config->server()->hostName(),
                                                  config->server()->http()->port());

            srvThread = std::thread([this] {
                app_->runUntilExit(); //
            });

            while (!app_->isStarted()) {
                spdlog::info("Waiting for server to start...");
                usleep(1000 * 100); // 100ms
                if (app_->isExited()) {
                    BOOST_FAIL("Server exited unexpectedly during startup.");
                }
            }


            spdlog::info("Test server started on port {}", config->server()->http()->port());
        } catch (const std::exception &ex) {
            printNestedException(ex);
            BOOST_FAIL("Exception starting test server");
        } catch (...) {
            spdlog::critical("Unknown error starting test server.");
            BOOST_FAIL("Exception starting test server");
        }
        spdlog::info("Starting test server done.");
    }

    ~X402ServerFixture() {
        if (app_) app_->stopServer();
        if (srvThread.joinable()) srvThread.join();
    }


    std::shared_ptr<MachinePayApp> app_;
    std::shared_ptr<X402Client> client;

    std::thread srvThread;
    uint16_t port{0};
};


// Use the fixture for all tests in this suite
BOOST_FIXTURE_TEST_SUITE(X402Suite, X402ServerFixture)

    BOOST_AUTO_TEST_CASE(Returns402WhenNoPaymentHeader) {
        auto [headersMap, statusLine, resp] = client->sendRequestAndParseResult(
            "/posts/1", {}, true);


        BOOST_TEST(resp.status == 402);
        BOOST_TEST(statusLine == "HTTP/1.1 402 Payment Required");
        BOOST_TEST(headersMap["Content-Type"] == "application/json");

        PaymentRequiredResponse response;

        try {
            response = PaymentRequiredResponse::fromJson(nlohmann::json::parse(resp.body));
        } catch (const std::exception &ex) {
            printNestedException(ex);
            BOOST_FAIL("Failed to parse 402 response body as PaymentRequiredResponse");
        }
        auto accepts = response.accepts();
        BOOST_CHECK(accepts.size() == 1);
        auto req = accepts.front();
        auto expected = PaymentRequirements::fromJson(
            nlohmann::json::parse(PaymentExamples::EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA));
        BOOST_TEST(req == *expected);
    }

    BOOST_AUTO_TEST_CASE(Returns200WhenPaymentHeaderPresent) {
        EthAddress to("0x209693bc6afc0c5328ba36faf03c514ef312287c");
        EIP3009Value value(12000000000000000000ULL);


        EIP3009Nonce nonce = EIP3009Nonce::generateRandomNonce();
        std::string privKeyHex = "4c0883a69102937d6231471b5dbb6204fe5129617082796e8a7a7e7a7a7a7a7a";
        EthPrivateKey privKey(privKeyHex);


        auto paymentPayload = PaymentPayload().createDefaultPaymentPayload(
            privKey,
            to,
            value,
            nonce,
            "base-sepolia"
        );

        auto [headersMap, statusLine, resp] =
            client->sendRequestWithPayloadAndParseResult("/posts/1", paymentPayload, true);


        BOOST_TEST(resp.status == 200);
        BOOST_TEST(headersMap.contains("X-PAYMENT-RESPONSE"));
        auto paymentResponse = headersMap.at("X-PAYMENT-RESPONSE");
        BOOST_TEST(resp.body.size() > 0);

        // this should cause exception
        auto [headersMap2, statusLine2, resp2] = client->sendRequestWithPayloadAndParseResult(
            "/posts/1", paymentPayload, true);

        BOOST_TEST(resp2.status == 402);
        BOOST_TEST(headersMap2.contains("X-PAYMENT-RESPONSE"));
        BOOST_TEST(resp2.body.size() > 0);


        unsetenv("TEST_DISABLE_AUTHORIZATION_TIME_CHECK");
    }

BOOST_AUTO_TEST_SUITE_END()
