#include "MachinePayCommon.h"
#include "Authorization.h"
#include "x402_protocol/HttpError.h"
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <boost/algorithm/hex.hpp>

Authorization::Authorization() = default;

Authorization::Authorization(const std::string &fromStr,
                             const std::string &toStr,
                             const std::string &value,
                             const std::string &validAfter,
                             const std::string &validBefore,
                             const std::string &nonce) :
      value_(value),
      validAfter_(validAfter),
      validBefore_(validBefore),
      nonce_(nonce) {
    from_ = Address::parseHexAddress(fromStr);
    to_ = Address::parseHexAddress(toStr);
    fromHex_ = from_.toHex(); // normalized 0x lowercase
    toHex_ = to_.toHex();
}

const std::string &Authorization::value() const { return value_; }
const std::string &Authorization::validAfter() const { return validAfter_; }
const std::string &Authorization::validBefore() const { return validBefore_; }
const std::string &Authorization::nonce() const { return nonce_; }


bool Authorization::operator==(const Authorization &other) const {
    return from_ == other.from_ &&
           to_ == other.to_ &&
           value_ == other.value_ &&
           validAfter_ == other.validAfter_ &&
           validBefore_ == other.validBefore_ &&
           nonce_ == other.nonce_;
}

std::shared_ptr<Authorization> Authorization::fromJson(const json &j) {
    CHECK_STATE_JSON(j.contains("from"), "Missing required field 'from' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("to"), "Missing required field 'to' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("value"), "Missing required field 'value' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("validAfter"), "Missing required field 'validAfter' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("validBefore"), "Missing required field 'validBefore' in Authorization JSON", j);
    CHECK_STATE_JSON(j.contains("nonce"), "Missing required field 'nonce' in Authorization JSON", j);

    CHECK_STATE_JSON(j.at("from").is_string(), "'from' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("to").is_string(), "'to' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("value").is_string(), "'value' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("validAfter").is_string(), "'validAfter' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("validBefore").is_string(), "'validBefore' must be a string in Authorization JSON", j);
    CHECK_STATE_JSON(j.at("nonce").is_string(), "'nonce' must be a string in Authorization JSON", j);

    return std::make_shared<Authorization>(
        j.at("from").get<std::string>(),
        j.at("to").get<std::string>(),
        j.at("value").get<std::string>(),
        j.at("validAfter").get<std::string>(),
        j.at("validBefore").get<std::string>(),
        j.at("nonce").get<std::string>()
    );
}

json Authorization::toJson() const {
    json j;
    j["from"] = fromHex_;
    j["to"] = toHex_;
    j["value"] = value_;
    j["validAfter"] = validAfter_;
    j["validBefore"] = validBefore_;
    j["nonce"] = nonce_;
    return j;
}

std::optional<HttpError> Authorization::validate(const MachinePayConfig &config, const ResourceConfig &resource) {
    // Check validAfter is less than or equal to current time
    // Check validBefore is greater than current time
    try {
        std::time_t validAfterTs = std::stoll(validAfter_);
        std::time_t validBeforeTs = std::stoll(validBefore_);
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (validAfterTs > now) {
            return HttpError(ErrorType::ERR_BAD_REQUEST,
                             "Authorization not yet valid: current time (" + std::to_string(now) +
                             ") is less than validAfter (" + std::to_string(validAfterTs) + ")");
        }
        if (validBeforeTs < now) {
            return HttpError(ErrorType::ERR_BAD_REQUEST,
                             "Authorization expired: current time (" + std::to_string(now) + ") is after validBefore ("
                             + std::to_string(validBeforeTs) + ")");
        }
    } catch (const std::exception &e) {
        return HttpError(ErrorType::ERR_INTERNAL_SERVER_ERROR,
                         std::string("Authorization failed to validate") + e.what());
    }
    return std::nullopt; // no error
}


