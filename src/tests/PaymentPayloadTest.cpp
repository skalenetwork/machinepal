#include "../datastructures/PaymentPayload.h"
#include "../examples/PaymentExamples.h"

#include <boost/test/unit_test.hpp>
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

BOOST_AUTO_TEST_CASE(deserialize_payment_payload) {



    json j_data = json::parse(PaymentExamples::EXACT_UCDC_PAYMENT_PAYLOAD_CB_SEPOLIA);
    json j_payload = j_data["paymentPayload"];
    PaymentPayload p_payload = j_payload.get<PaymentPayload>();

    BOOST_TEST(p_payload.x402Version == 1);
    BOOST_TEST(p_payload.scheme == "exact");
    BOOST_TEST(p_payload.network == "base-sepolia");
    BOOST_TEST(p_payload.payload.signature == "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef1b");
    BOOST_TEST(p_payload.payload.authorization.from == "0x1111111111111111111111111111111111111111");
    BOOST_TEST(p_payload.payload.authorization.to == "0x2222222222222222222222222222222222222222");
    BOOST_TEST(p_payload.payload.authorization.value == "1000");
    BOOST_TEST(p_payload.payload.authorization.validAfter == "1716150000");
    BOOST_TEST(p_payload.payload.authorization.validBefore == "1716153600");
    BOOST_TEST(p_payload.payload.authorization.nonce == "0x1234567890abcdef");
}

BOOST_AUTO_TEST_CASE(serialize_payment_payload) {
    PaymentPayload new_payload;
    new_payload.x402Version = 2;
    new_payload.scheme = "streaming";
    new_payload.network = "optimism";
    new_payload.payload.signature = "0xfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1dead";
    new_payload.payload.authorization.from = "0x5555555555555555555555555555555555555555";
    new_payload.payload.authorization.to = "0x6666666666666666666666666666666666666666";
    new_payload.payload.authorization.value = "1000000000000000000";
    new_payload.payload.authorization.validAfter = "1727280000";
    new_payload.payload.authorization.validBefore = "1727283600";
    new_payload.payload.authorization.nonce = "0xfee1deadbeef";

    json j_output;
    j_output["paymentPayload"] = new_payload;

    BOOST_TEST(j_output["paymentPayload"]["x402Version"] == 2);
    BOOST_TEST(j_output["paymentPayload"]["scheme"] == "streaming");
    BOOST_TEST(j_output["paymentPayload"]["network"] == "optimism");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["signature"] == "0xfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1deadfee1dead");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["from"] == "0x5555555555555555555555555555555555555555");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["to"] == "0x6666666666666666666666666666666666666666");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["value"] == "1000000000000000000");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["validAfter"] == "1727280000");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["validBefore"] == "1727283600");
    BOOST_TEST(j_output["paymentPayload"]["payload"]["authorization"]["nonce"] == "0xfee1deadbeef");
}

