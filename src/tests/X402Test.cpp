// X402HandlerBoostTest.cpp
#define BOOST_TEST_MODULE X402HandlerSelfTest

#include "MachinePayCommon.h"
#include "init/Init.h"
#include "x402_server/ServerFactory.h"
#include <boost/test/included/unit_test.hpp>  // or <boost/test/unit_test.hpp> if using dynamic link

#include "x402_client/X402Client.h"


#include "../examples/PaymentExamples.h"
#include "../payment/datastructures/PaymentRequiredResponse.h"
#include "../payment/datastructures/PaymentRequirements.h"
#include "MachinePayApp.h"
#include "config/ConfigLoader.h"
#include "config/ConfigManager.h"
#include "config/subconfigs/ServerConfig.h"
#include "examples/PaymentExamples.h"
#include "url/URLUtils.h"
#include <curl/curl.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstdlib>
#include <sys/wait.h>


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
                {
                    "CONFIG",
                    "src/tests/configs/basic/machinepay.yml"
                }
            };
            app_ = MachinePayApp::makeInstance(configMap);
            auto config = app_->configManager()->latestConfig();
            auto url = "https://" + config->server()->hostName() + ":" +
                       std::to_string(config->server()->https()->port());
            client_ = std::make_shared<X402Client>();
            baseUrl_ = url;


            srvThread_ = std::thread([this] {
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
            spdlog::set_level(spdlog::level::trace);
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
        if (app_)
            app_->stopServer();
        if (srvThread_.joinable())
            srvThread_.join();
    }


    std::shared_ptr<MachinePayApp> app_;
    std::shared_ptr<X402Client> client_;
    std::string baseUrl_;

    std::thread srvThread_;
    uint16_t port_{0};
};


// Use the fixture for all tests in this suite
BOOST_FIXTURE_TEST_SUITE(X402Suite, X402ServerFixture)

    BOOST_AUTO_TEST_CASE(Returns402WhenNoPaymentHeader) {
        auto resp = client_->doX402Request(proxygen::HTTPMethod::GET,
            baseUrl_ + "/posts/1", nullptr,
            nullptr);


        resp.headers.forEach([](const std::string &name, const std::string &value) {
            spdlog::info("{}: {}", name, value);
        });
        spdlog::info("BODY::{}", resp.body);


        BOOST_TEST(resp.status == 402);
        BOOST_TEST(resp.headers.getSingleOrEmpty("Content-Type") == "application/json");

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
            nlohmann::json::parse(PaymentExamples::EXACT_UCDC_PAYMENT_REQ_EASYNET));
        BOOST_TEST(req == *expected);
    }

    BOOST_AUTO_TEST_CASE(Returns200WhenPaymentHeaderPresent) {
        EthAddress to("0x209693bc6afc0c5328ba36faf03c514ef312287c");
        EIP3009Value value(12000000000000000000ULL);
        EIP3009Nonce nonce = EIP3009Nonce::generateRandomNonce();
        std::string privKeyHex = "4c0883a69102937d6231471b5dbb6204fe5129617082796e8a7a7e7a7a7a7a7a";
        EthPrivateKey privKey(privKeyHex);


        auto paymentPayload =
                PaymentPayload().createDefaultPaymentPayload(privKey, to,
                                                             value, nonce, "machinepay-easynet");

        sleep(1);

        auto resp =
                client_->doX402Request(proxygen::HTTPMethod::GET, baseUrl_
                                          + "/posts/1", paymentPayload, nullptr);


        BOOST_TEST(resp.status == 200);
        BOOST_TEST(resp.headers.exists( "X-PAYMENT-RESPONSE" ));
        auto paymentResponse = resp.headers.getSingleOrEmpty("X-PAYMENT-RESPONSE");
        BOOST_TEST(resp.body.size() > 0);

        // this should cause exception
        auto resp2 =
                client_->doX402Request(proxygen::HTTPMethod::GET, baseUrl_ + "/posts/1", paymentPayload,
                    nullptr);

        BOOST_TEST(resp2.status == 402);
        BOOST_TEST(resp2.headers.exists( "X-PAYMENT-RESPONSE" ));
        BOOST_TEST(resp2.body.size() > 0);


        unsetenv("TEST_DISABLE_AUTHORIZATION_TIME_CHECK");
    }

    BOOST_AUTO_TEST_CASE(ClientCliRunsExecutable) {
        // Determine path to machinepay executable relative to this test binary
        namespace fs = std::filesystem;
        auto& ts = boost::unit_test::framework::master_test_suite();
        fs::path testExePath(ts.argv[0]);
        fs::path exeDir = testExePath.parent_path();
        fs::path machinepayPath = exeDir / "../machinepay";
        // In some setups executables are in the same directory
        if (!fs::exists(machinepayPath)) {
            machinepayPath = exeDir / "machinepay";
        }
        BOOST_REQUIRE_MESSAGE(fs::exists(machinepayPath), std::string("machinepay executable not found at ") + machinepayPath.string());

        std::string url = baseUrl_ + "/posts/1";
        std::string cmd = std::string("\"") + machinepayPath.string() + "\" client --method GET --url \"" + url + "\"";
        int rc = std::system(cmd.c_str());
        int exitCode = -1;
        if (WIFEXITED(rc)) {
            exitCode = WEXITSTATUS(rc);
        } else {
            exitCode = rc;
        }
        BOOST_TEST(exitCode == 0);
    }

    BOOST_AUTO_TEST_CASE(InitProjectGeneratesFilesInTmp) {
        namespace fs = std::filesystem;
        auto& ts = boost::unit_test::framework::master_test_suite();
        fs::path testExePath(ts.argv[0]);
        fs::path exeDir = testExePath.parent_path();
        fs::path machinepayPath = exeDir / "../machinepay";
        if (!fs::exists(machinepayPath)) {
            machinepayPath = exeDir / "machinepay";
        }
        BOOST_REQUIRE_MESSAGE(fs::exists(machinepayPath), std::string("machinepay executable not found at ") +
            machinepayPath.string());

        // Prepare empty directory at /tmp/machinepay
        fs::path baseDir("/tmp/machinepay");

        std::error_code ec;

        // Clean up to keep environment tidy (best-effort)
        fs::remove_all(baseDir, ec);


        if (fs::exists(baseDir, ec)) {
            fs::remove_all(baseDir, ec);
        }
        fs::create_directories(baseDir, ec);
        BOOST_REQUIRE_MESSAGE(!ec && fs::is_empty(baseDir), "Failed to prepare empty /tmp/machinepay directory");

        std::string cmd = "cd /tmp/machinepay && \"" + machinepayPath.string() + "\" init";
        int rc = std::system(cmd.c_str());
        int exitCode = -1;
        if (WIFEXITED(rc)) {
            exitCode = WEXITSTATUS(rc);
        } else {
            exitCode = rc;
        }
        BOOST_REQUIRE_MESSAGE(exitCode == 0, std::string("machinepay --init-project failed with exit code ") + std::to_string(exitCode));

        // Verify generated structure
        fs::path cfg = baseDir / "machinepay.yml";
        fs::path secrets = baseDir / "secrets";
        fs::path certs = baseDir / "certs";
        fs::path wallet = secrets / "machinepay_wallet.key";
        fs::path cert = certs / "machinepay_tls_certificate.crt";
        fs::path certKey = secrets / "machinepay_tls_certificate.key";

        BOOST_TEST(fs::exists(cfg));
        BOOST_TEST(fs::exists(secrets));
        BOOST_TEST(fs::exists(certs));
        BOOST_TEST(fs::exists(wallet));
        BOOST_TEST(fs::exists(cert));
        BOOST_TEST(fs::exists(certKey));

    }

BOOST_AUTO_TEST_SUITE_END()
