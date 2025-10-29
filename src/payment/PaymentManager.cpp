#include "MachinePayCommon.h"
#include "PaymentManager.h"
#include "MachinePayApp.h"
#include "config/subconfigs/FacilitatorConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/ResourceConfig.h"
#include "datastructures/PaymentPayload.h"
#include "db/MachinePayDB.h"
#include "db/PaymentRecord.h"
#include "url/URLUtils.h"

PaymentManager::PaymentManager(MachinePayApp &app) : app_(app) {
}

variant<ptr<PaymentPayload>, HttpError> PaymentManager::decodeAndParsePayment(
    const std::unique_ptr<proxygen::HTTPMessage> &req) {
    try {
        const auto &headerTable = req->getHeaders();
        std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
        // the payment header should not be empty at this point - otherwise we would have replied 402 already
        CHECK_STATE(!payment.empty());

        std::string decoded;
        try {
            decoded = URLUtils::base64Decode(payment);
        } catch (const std::exception &e) {
            spdlog::error("Exception during base64 decode of X-PAYMENT header: {}", e.what());
            return HttpError(ERR_BAD_REQUEST, "X-PAYMENT header is not valid base64");
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(decoded);
        } catch (const std::exception &e) {
            spdlog::error("Failed to parse decoded X-PAYMENT header as JSON: {} {}", decoded, e.what());
            return HttpError(ERR_BAD_REQUEST, "X-PAYMENT header is not valid JSON after base64 decoding");
        }

        auto paymentPayloadP = PaymentPayload::fromJson(j);
        CHECK_STATE(paymentPayloadP);
        return paymentPayloadP;
    } catch (std::exception &e) {
        return HttpError(ERR_INTERNAL_SERVER_ERROR,
                         std::string("Error parsing X-PAYMENT header: ") + e.what());
    }
}

void PaymentManager::recordSuccessfulSettlement(const PaymentPayload &payload, const ResourceConfig &resource) {
    auto db = app_.machinePayDB();


    auto paymentRecord = PaymentRecord::createPaymentRecordFromPaymentPayloadAndResource(payload, resource);
    CHECK_STATE(paymentRecord);
    db->writePayment(*paymentRecord);
}

// Before
std::optional<HttpError> PaymentManager::checkAgaistAlreadySettledPayments(const ptr<PaymentPayload> &paymentPayload,
                                                                           const ptr<EIP712Domain> &domain) {
    auto db = app_.machinePayDB();
    auto from = paymentPayload->payload()->authorization()->from();
    auto nonce = paymentPayload->payload()->authorization()->nonce();
    auto asset = domain->assetAddress();
    auto chainId = domain->chainId();

    if (db->paymentExists(from, asset, nonce, chainId)) {
        spdlog::info("Payment has already been spent: from={}, nonce={}, token={}, tokenAddress={}, chainId={}",
                     from.toHex(true),
                     nonce.toHex(),
                     domain->name(),
                     asset.toHex(true),
                     chainId.str());
        return HttpError(ERR_BAD_REQUEST,
                         std::string("This payment has already been spent: ") +
                         "from=" + from.toHex(true) +
                         ", nonce=" + nonce.toHex() +
                         ", token:" + domain->name() +
                         ", tokenAddress=" + asset.toHex(true) +
                         ", chainId=" + chainId.str());
    }

    return std::nullopt;
}


std::optional<HttpError> PaymentManager::decodeValidateAndSettlePayment(
    const std::unique_ptr<proxygen::HTTPMessage> &req,
    std::string &settlementInfo,
    const MachinePayConfig &config,
    const ResourceConfig &resource) {
    try {
        auto result = decodeAndParsePayment(req);
        if (holds_alternative<HttpError>(result)) {
            return std::get<HttpError>(result);
        }

        auto paymentPayload = std::get<ptr<PaymentPayload> >(result);


        auto error = paymentPayload->validateAndVerifySignature(config, resource);

        if (error) {
            return error;
        }

        auto domain = config.network()->eip712Domain();

        error = checkAgaistAlreadySettledPayments(paymentPayload, domain);

        if (error) {
            return error;
        }

        auto facilitator = config.network()->facilitator();

        error = facilitator->settlePayment(paymentPayload, settlementInfo);

        if (error) {
            return error;
        }

        recordSuccessfulSettlement(*paymentPayload, resource);

        return std::nullopt;
    } catch (std::exception &e) {
        spdlog::error("validatePayment had exception while parsing X-PAYMENT header: {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("Error parsing X-PAYMENT header: ") + e.what());
    }
}
