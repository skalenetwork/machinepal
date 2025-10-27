#include "PaymentManager.h"
#include "MachinePayApp.h"
#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/ResourceConfig.h"
#include "datastructures/PaymentPayload.h"
#include "url/URLUtils.h"

PaymentManager::PaymentManager(MachinePayApp& app) : app_(app) {}

variant<ptr<PaymentPayload>, HttpError> PaymentManager::decodeAndParsePayment(const std::unique_ptr<proxygen::HTTPMessage> &req) {
    try {
        const auto &headerTable = req->getHeaders();
        std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
        // the payment header should not be empty at this point - otherwise we would have replied 402 already
        CHECK_STATE(!payment.empty());

        std::string decoded;
        try {
            decoded = URLUtils::base64Decode(payment);
        } catch (const std::exception& e) {
            spdlog::error("Exception during base64 decode of X-PAYMENT header: {}", e.what());
            return HttpError(ERR_BAD_REQUEST, "X-PAYMENT header is not valid base64");
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(decoded);
        } catch (const std::exception& e) {
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

std::optional<HttpError> PaymentManager::decodeValidateAndSettlePayment(const std::unique_ptr<proxygen::HTTPMessage> &req,
                                                                        std::string &settlementInfo,
                                                                        const MachinePayConfig& config,
                                                                        const ResourceConfig& resource) {
    (void)config; // currently unused
    (void)resource; // currently unused
    try {

        auto result = decodeAndParsePayment(req);
        if (holds_alternative<HttpError>(result)) {
            return std::get<HttpError>(result);
        }

        auto paymentPayload = std::get<ptr<PaymentPayload>>(result);

        std::optional<HttpError> error = paymentPayload->validateAndVerifySignature(config, resource);

        if (error) {
            return error;
        }


        if (error) {
            return error;
        }

        try {
            settlementInfo = paymentPayload->toJson().dump();
        } catch (const std::exception& e) {
            spdlog::error("Failed serializing payment payload to JSON: {}", e.what());
            return HttpError(ERR_INTERNAL_SERVER_ERROR, "Failed serializing payment payload");
        }

        return std::nullopt; // success
    } catch (std::exception &e) {
        spdlog::error("validatePayment had exception while parsing X-PAYMENT header: {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("Error parsing X-PAYMENT header: ") + e.what());
    }
}
