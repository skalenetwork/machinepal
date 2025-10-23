// X402HandlerBoostTest.cpp
#define BOOST_TEST_MODULE X402HandlerSelfTest

#include "MachinePayCommon.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"
#include <boost/test/included/unit_test.hpp> // or <boost/test/unit_test.hpp> if using dynamic link
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>


#include "x402_client/X402Client.h"


// ---- libcurl helper ---------------------------------------------------------
#include <boost/beast/core/detail/base64.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>


#include "MachinePayApp.h"
#include "../payment/PaymentRequirements.h"
#include "../examples/PaymentExamples.h"
#include "config/ConfigLoader.h"
#include "config/ConfigManager.h"
#include "config/subconfigs/ServerConfig.h"
#include "payment/PaymentRequiredResponse.h"
#include "url/URLUtils.h"


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


// ---- Test fixture that starts/stops the proxygen server ---------------------
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
    auto expected = PaymentRequirements::fromJson(nlohmann::json::parse(EXACT_UCDC_PAYMENT_REQ_CB_SEPOLIA));
        BOOST_TEST(req == *expected);
    }

    BOOST_AUTO_TEST_CASE(Returns200WhenPaymentHeaderPresent) {
        std::string xPaymentValue =
            R"({"txHash":"0xabc123...","amount":"0.25","asset":"SDC","network":"base-1net"})";
        std::string xPaymentBase64 = URLUtils::base64Encode(xPaymentValue);
        auto [headersMap, statusLine, resp] = client->sendRequestAndParseResult("/posts/1",
            {"X-PAYMENT: " + xPaymentBase64}, true);
        BOOST_TEST(resp.status == 200);
        auto xPaymentTesponse = headersMap.at("X-PAYMENT-RESPONSE");
        BOOST_TEST(xPaymentTesponse.find("txHash") != std::string::npos);
        BOOST_TEST(resp.body.size() > 0);
    }

BOOST_AUTO_TEST_SUITE_END()
