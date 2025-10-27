#include "../payment/datastructures/PaymentPayload.h"
#include "../examples/PaymentExamples.h"

#include <boost/test/unit_test.hpp>
#include <string>
#include <memory>
#include "nlohmann/json.hpp"

using json = nlohmann::json;


auto SIGNATURE_SAMPLE =
        "0x2d6a7588d6acca505cbf0d9a4a227e0c52c6c34008c8e8986a1283259764173608a2ce6496642e377d6da8dbbf5836e9bd15092f9ecab05ded3d6293af148b571c";

BOOST_AUTO_TEST_CASE(deserialize_payment_payload) {
    json jData = json::parse(PaymentExamples::EXACT_UCDC_PAYMENT_PAYLOAD_CB_SEPOLIA);
    PaymentPayload paymentPayload = *PaymentPayload::fromJson(jData);


    BOOST_TEST(paymentPayload.x402Version() == 1);
    BOOST_TEST(paymentPayload.scheme() == "exact");
    BOOST_TEST(paymentPayload.network() == "base-sepolia");
    BOOST_TEST(paymentPayload.payload()->signature().toHex(true) == SIGNATURE_SAMPLE);
    BOOST_TEST(
        paymentPayload.payload()->authorization()->from().toHex() == "0x857b06519E91e3A54538791bDbb0E22373e36b66");
    BOOST_TEST(paymentPayload.payload()->authorization()->to().toHex() == "0x209693Bc6afc0C5328bA36FaF03C514EF312287C");
    BOOST_TEST(paymentPayload.payload()->authorization()->value().toDecimal() == "10000");
    BOOST_TEST(paymentPayload.payload()->authorization()->validAfter().toDecimal() == "1740672089");
    BOOST_TEST(paymentPayload.payload()->authorization()->validBefore().toDecimal() == "1740672154");
    BOOST_TEST(paymentPayload.payload()->authorization()->nonce().toHex(true) ==
        "0xf3746613c2d920b5fdabc0856f2aeb2d4f88ee6037b8cc5d04a71a4462f13480");
}

BOOST_AUTO_TEST_CASE(serialize_payment_payload) {
    auto auth = std::make_shared<Authorization>(
        "0x857b06519E91e3A54538791bDbb0E22373e36b66",
        "0x209693Bc6afc0C5328bA36FaF03C514EF312287C",
        "10000",
        "1740672089",
        "1740672154",
        "0xf3746613c2d920b5fdabc0856f2aeb2d4f88ee6037b8cc5d04a71a4462f13480"
    );
    auto payloadPtr = std::make_shared<Payload>(SIGNATURE_SAMPLE,
                                                auth
    );
    PaymentPayload newPayload(2, "exact", "base-sepolia", payloadPtr);

    json jOutput;
    jOutput["paymentPayload"] = newPayload.toJson();

    BOOST_TEST(jOutput["paymentPayload"]["x402Version"] == 2);
    BOOST_TEST(jOutput["paymentPayload"]["scheme"] == "exact");
    BOOST_TEST(jOutput["paymentPayload"]["network"] == "base-sepolia");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["signature"] == SIGNATURE_SAMPLE);
    BOOST_TEST(
        jOutput["paymentPayload"]["payload"]["authorization"]["from"] == "0x857b06519E91e3A54538791bDbb0E22373e36b66");
    BOOST_TEST(
        jOutput["paymentPayload"]["payload"]["authorization"]["to"] == "0x209693Bc6afc0C5328bA36FaF03C514EF312287C");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["value"] == "10000");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["validAfter"] == "1740672089");
    BOOST_TEST(jOutput["paymentPayload"]["payload"]["authorization"]["validBefore"] == "1740672154");
    BOOST_TEST(
        jOutput["paymentPayload"]["payload"]["authorization"]["nonce"] ==
        "0xf3746613c2d920b5fdabc0856f2aeb2d4f88ee6037b8cc5d04a71a4462f13480");
}
