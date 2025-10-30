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

PaymentManager::PaymentManager(MachinePayApp &app)
    : app_(app) {
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

void PaymentManager::recordSuccessfulSettlement(const PaymentPayload &payload,
                                                const EIP712Domain &domain,
                                                const ResourceConfig &resource) {
    auto db = app_.machinePayDB();
    db->saveSettledPayment(payload, domain, resource);
}


std::optional<HttpError> PaymentManager::checkAgainstAlreadySettledPayments(const ptr<PaymentPayload> &paymentPayload,
                                                                            const ptr<EIP712Domain> &domain) {
    auto db = app_.machinePayDB();

    if (db->settledPaymentExists(paymentPayload, domain)) {

        const auto &from = paymentPayload->payload()->authorization()->from();
        const auto &nonce = paymentPayload->payload()->authorization()->nonce();
        const auto &asset = domain->assetAddress();
        const auto &chainId = domain->chainId();

        spdlog::info("Payment has already been spent: from={}, nonce={}, token={}, tokenAddress={}, chainId={}",
                     from.toHex(PREFIX_0x),
                     nonce.toHex(),
                     domain->name(),
                     asset.toHex(PREFIX_0x),
                     chainId.str());
        return HttpError(ERR_BAD_REQUEST,
                         std::string("This payment has already been spent: ") +
                         "from=" + from.toHex(PREFIX_0x) +
                         ", nonce=" + nonce.toHex() +
                         ", token:" + domain->name() +
                         ", tokenAddress=" + asset.toHex(PREFIX_0x) +
                         ", chainId=" + chainId.str());
    }

    return std::nullopt;
}


bool PaymentManager::lockPaymentAsBeingSettled(ptr<Authorization> authorization, ptr<EIP712Domain> domain) {
    auto uniquePaymentKey = domain->uniquePaymentKey(authorization->from(), authorization->nonce());
    lock_guard<std::mutex> lock(currentlySettlingPaymentsMutex_);
    auto result = currentlySettlingPayments_.insert(uniquePaymentKey);
    auto lockedPayment = result.second;
    return lockedPayment;
}

void PaymentManager::unlockPaymentAsBeingSettled(ptr<Authorization> authorization, ptr<EIP712Domain> domain) {
    auto uniquePaymentKey = domain->uniquePaymentKey(authorization->from(), authorization->nonce());
    lock_guard<std::mutex> lock(currentlySettlingPaymentsMutex_);
    CHECK_STATE(currentlySettlingPayments_.erase(uniquePaymentKey) == 1);
}


// this function assumes the payment has been locked for settlement already
std::optional<HttpError> PaymentManager::checkPaymentIsNewAndSettleItUnsafe(std::string &settlementInfo,
                                                      const NetworkConfig &networkConfig,
                                                      const ResourceConfig &resource,
                                                      shared_ptr<PaymentPayload> paymentPayload) {
    std::optional<HttpError> error = std::nullopt;

    error = checkAgainstAlreadySettledPayments(paymentPayload, networkConfig.eip712Domain());

    if (error) {
        return error;
    }

    auto facilitator = networkConfig.facilitator();

    error = facilitator->settlePayment(paymentPayload, settlementInfo);

    if (error) {
        return error;
    }

    recordSuccessfulSettlement(*paymentPayload, *networkConfig.eip712Domain(), resource);

    return std::nullopt;
}

std::optional<HttpError> PaymentManager::checkPaymentIsNewAndSettleIt(std::string &settlementInfo,
                                                                      const NetworkConfig &networkConfig,
                                                                      const ResourceConfig &resource,
                                                                      shared_ptr<PaymentPayload> paymentPayload) {
    CHECK_STATE(paymentPayload);
    auto authorization = paymentPayload->payload()->authorization();
    auto domain = networkConfig.eip712Domain();

    // Try to acquire the lock for this payment
    if (!lockPaymentAsBeingSettled(authorization, domain)) {
        // payment has already been submitted and is being processed
        // by a different thread. Return immediately.
        return HttpError(ERR_BAD_REQUEST, "Your payment is currently being processed. "
                         "Looks like you submitted the same payment twice.");
    }

    std::optional<HttpError> error;

    try {
        // Call the settleUnsafe function tjhat assumes that we hold the lock
        error = checkPaymentIsNewAndSettleItUnsafe(settlementInfo, networkConfig, resource, paymentPayload);
    } catch (const std::exception &e) {
        spdlog::error("settleUnsafe had an exception: {}", e.what());
        error = HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("Error during payment settlement: ") + e.what());
    } catch (...) {
        spdlog::error("settleUnsafe had an unknown exception");
        error = HttpError(ERR_INTERNAL_SERVER_ERROR, "Unknown error during payment settlement");
    }

    unlockPaymentAsBeingSettled(authorization, domain);
    return error;
}


std::optional<HttpError> PaymentManager::decodeValidateAndSettlePayment(
    const std::unique_ptr<proxygen::HTTPMessage> &req,
    std::string &settlementInfo,
    const MachinePayConfig &config,
    const ResourceConfig &resource) {
    std::optional<HttpError> error = std::nullopt;
    std::string uniquePaymentKey;

    try {
        auto result = decodeAndParsePayment(req);
        if (holds_alternative<HttpError>(result)) {
            return std::get<HttpError>(result);
        }

        auto paymentPayload = std::get<ptr<PaymentPayload>>(result);

        auto authorization = paymentPayload->payload()->authorization();;

        error = paymentPayload->validateAndVerifySignature(config, resource);

        if (error)
            return error;

        return checkPaymentIsNewAndSettleIt(settlementInfo, *config.network(), resource, paymentPayload);
    } catch (std::exception &e) {
        spdlog::error("decodeValidateAndSettlePayment had exception  {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("decodeValidateAndSettlePayment had exception")
                                                    + e.what());
    }
}