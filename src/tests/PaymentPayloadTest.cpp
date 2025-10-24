#include "../payment/datastructures/PaymentPayload.h"
#include "../examples/PaymentExamples.h"

#include <boost/test/unit_test.hpp>
#include <string>
#include <memory>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

BOOST_AUTO_TEST_CASE(deserialize_payment_payload) {



    json jData = json::parse(PaymentExamples::EXACT_UCDC_PAYMENT_PAYLOAD_CB_SEPOLIA);
    PaymentPayload paymentPayload = *PaymentPayload::fromJson(jData);

    BOOST_TEST(paymentPayload.x402Version() == 1);
    BOOST_TEST(paymentPayload.scheme() == "exact");
    BOOST_TEST(paymentPayload.network() == "base-sepolia");
    BOOST_TEST(paymentPayload.payload()->signature() == "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef1b");
    BOOST_TEST(paymentPayload.payload()->authorization()->from().toHex() == "0x1111111111111111111111111111111111111111");
    BOOST_TEST(paymentPayload.payload()->authorization()->to().toHex() == "0x2222222222222222222222222222222222222222");
    BOOST_TEST(paymentPayload.payload()->authorization()->value() == "1000");
    BOOST_TEST(paymentPayload.payload()->authorization()->validAfter() == "1716150000");
    BOOST_TEST(paymentPayload.payload()->authorization()->validBefore() == "1716153600");
    BOOST_TEST(paymentPayload.payload()->authorization()->nonce() == "0x1234567890abcdef");
}

BOOST_AUTO_TEST_CASE(serialize_payment_payload) {
    auto auth = std::make_shared<Authorization>(
        "0x5555555555555555555555555555555555555555",
        "0x6666666666666666666666666666666666666666",
        "1000000000000000000",
        "1727280000",
        "1727283600",
        "0xfee1deadbeef"
    );
    auto payloadPtr = std::make_shared<Payload>(
        "0xfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1dead",
        auth
    );
    PaymentPayload newPayload(2, "streaming", "optimism", payloadPtr);

    json jOutput;
    jOutput["paymentPayload"] = newPayload.toJson();

    BOOST_TEST(jOutput["paymentPayload"]["x402Version"] == 2);
    BOOST_TEST(jOutput["paymentPayload"]["scheme"] == "streaming");
    BOOST_TEST(jOutput["paymentPayload"]["network"] == "optimism");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["signature"] == "0xfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1dead");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["from"] == "0x5555555555555555555555555555555555555555");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["to"] == "0x6666666666666666666666666666666666666666");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["value"] == "1000000000000000000");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["validAfter"] == "1727280000");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["validBefore"] == "1727283600");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["nonce"] == "0xfee1deadbeef");
}
