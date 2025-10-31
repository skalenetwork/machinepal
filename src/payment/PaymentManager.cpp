#include "MachinePayCommon.h"
#include "PaymentManager.h"
#include "MachinePayApp.h"
#include "config/subconfigs/FacilitatorConfig.h"
#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/ResourceConfig.h"
#include "crypto/Keccak.h"
#include "datastructures/PaymentPayload.h"
#include "datastructures/SettlementResponse.h"
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
                                                const ResourceConfig &resource, const OrganizationConfig &organization,
                                                const Hash& transactionHash, const string& ipAddress) {
    auto db = app_.machinePayDB();
    db->saveSettledPayment(payload, domain, resource, organization, transactionHash, ipAddress);
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
                     nonce.toHex(PREFIX_0x),
                     domain->name(),
                     asset.toHex(PREFIX_0x),
                     chainId.str());
        return HttpError(ERR_BAD_REQUEST,
                         std::string("This payment has already been spent: ") +
                         "from=" + from.toHex(PREFIX_0x) +
                         ", nonce=" + nonce.toHex(PREFIX_0x) +
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
                                                                            const OrganizationConfig &organization,
                                                                            shared_ptr<PaymentPayload> paymentPayload,
                                                                            const string& ipAddress) {
    std::optional<HttpError> error = std::nullopt;

    error = checkAgainstAlreadySettledPayments(paymentPayload, networkConfig.eip712Domain());

    if (error) {
        return error;
    }

    auto facilitator = networkConfig.facilitator();

    auto settleResult = facilitator->settlePayment(paymentPayload);
    if (holds_alternative<HttpError>(settleResult)) {
        return std::get<HttpError>(settleResult);
    }
    const auto &settlementResponse = std::get<SettlementResponse>(settleResult);

    auto transactionHash = Encoding::fromHexToHash(settlementResponse.transaction());

    recordSuccessfulSettlement(*paymentPayload, *networkConfig.eip712Domain(), resource, organization, transactionHash,
        ipAddress);

    return std::nullopt;
}

std::optional<HttpError> PaymentManager::checkPaymentIsNewAndSettleIt(std::string &settlementInfo,
                                                                      const NetworkConfig &networkConfig,
                                                                      const ResourceConfig &resource,
                                                                      const OrganizationConfig &organization,
                                                                      shared_ptr<PaymentPayload> paymentPayload,
                                                                      const string& ipAddress) {
    CHECK_STATE(paymentPayload);
    auto authorization = paymentPayload->payload()->authorization();
    auto domain = networkConfig.eip712Domain();

    // we need to make sure that the user can not submit the same payment multiple times in parallel
    // submitting the same payment twice should result in one successful settlement and one error response
    // otherwise the user could use the same payment to pay for multiple requests
    // we do it by marking the payment as locked while being settled

    if (!lockPaymentAsBeingSettled(authorization, domain)) {
        // Failed to acquire lock, return immediately.
        return HttpError(ERR_BAD_REQUEST, "Your payment is currently being processed. "
                         "Looks like you submitted the same payment twice.");
    }

    // Lock acquired. Immediately create the RAII guard.
    // folly::makeGuard creates a guard that will unlock the payment once we exit this function,.
    auto guard = folly::makeGuard([&]() {
        unlockPaymentAsBeingSettled(authorization, domain);
    });

    try {
        // Now that we hold the lock, proceed with the actual settlement.
        return checkPaymentIsNewAndSettleItUnsafe(settlementInfo, networkConfig, resource, organization, paymentPayload,
            ipAddress);
    } catch (const std::exception &e) {
        spdlog::error("Error during payment settlement: {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("Error during payment settlement: ") + e.what());
    }
}


std::optional<HttpError> PaymentManager::decodeValidateAndSettlePayment(
    const std::unique_ptr<proxygen::HTTPMessage> &req,
    std::string &settlementInfo,
    const MachinePayConfig &config,
    const ResourceConfig &resource,
    const OrganizationConfig &organization) {
    std::optional<HttpError> error = std::nullopt;

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

        auto ipAddress = req->getClientAddress().getAddressStr();

        return checkPaymentIsNewAndSettleIt(settlementInfo, *config.network(), resource, organization, paymentPayload,
            ipAddress);
    } catch (std::exception &e) {
        spdlog::error("decodeValidateAndSettlePayment had exception  {}", e.what());
        return HttpError(ERR_INTERNAL_SERVER_ERROR, std::string("decodeValidateAndSettlePayment had exception")
                                                    + e.what());
    }
}