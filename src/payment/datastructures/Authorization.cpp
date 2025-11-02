#include "MachinePayCommon.h"
#include "Authorization.h"
#include "x402_protocol/HttpError.h"
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <boost/algorithm/hex.hpp>

#include "config/subconfigs/NetworkConfig.h"
#include "config/subconfigs/ResourceConfig.h"


Authorization::Authorization(const std::string& fromStr,
                             const std::string& toStr,
                             const std::string& value,
                             const std::string& validAfter,
                             const std::string& validBefore,
                             const std::string& nonce)
{
    value_ = EIP3009Value::fromHexOrDecimal(value);
    validAfter_ = EIP3009ValidityTime::fromHexOrDecimal(validAfter);
    validBefore_ = EIP3009ValidityTime::fromHexOrDecimal(validBefore);
    from_ = EthAddress::parseHexAddress(fromStr);
    to_ = EthAddress::parseHexAddress(toStr);
    nonce_ = EIP3009Nonce::fromHex(nonce);
}

const EIP3009Value& Authorization::value() const { return value_; }
const EIP3009ValidityTime& Authorization::validAfter() const { return validAfter_; }
const EIP3009ValidityTime& Authorization::validBefore() const { return validBefore_; }
const EIP3009Nonce& Authorization::nonce() const { return nonce_; }


bool Authorization::operator==(const Authorization& other) const
{
    return from_ == other.from_ &&
        to_ == other.to_ &&
        value_ == other.value_ &&
        validAfter_ == other.validAfter_ &&
        validBefore_ == other.validBefore_ &&
        nonce_ == other.nonce_;
}

std::shared_ptr<Authorization> Authorization::fromJson(const json& j)
{
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

    return std::make_shared < Authorization > (
        j.at("from").get<std::string>(),
        j.at("to").get<std::string>(),
        j.at("value").get<std::string>(),
        j.at("validAfter").get<std::string>(),
        j.at("validBefore").get<std::string>(),
        j.at("nonce").get<std::string>()
    );
}

json Authorization::toJson() const
{
    json j;
    j["from"] = from_.toChecksumHex();
    j["to"] = to_.toChecksumHex();
    j["value"] = value_.toDecimal();
    j["validAfter"] = validAfter_.toDecimal();
    j["validBefore"] = validBefore_.toDecimal();
    j["nonce"] = nonce_.toHex(PREFIX_0x);
    return j;
}

std::optional<HttpError> Authorization::checkValidityTime()
{
    // add disabling of valid time checks for testing so we can use fixed validAfter/validBefore values in tests
    if (std::getenv("TEST_DISABLE_AUTHORIZATION_TIME_CHECK"))
    {
        return std::nullopt;
    }

    auto now = EIP3009ValidityTime::fromTimeT(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    if (validAfter() > now)
    {
        return HttpError(ErrorType::ERR_BAD_REQUEST,
                         "Authorization not yet valid: current time (" + now.toDecimal() +
                         ") is less than validAfter (" + validAfter().toDecimal() + ")");
    }
    if (validBefore() < now)
    {
        return HttpError(ErrorType::ERR_BAD_REQUEST,
                         "Authorization expired: current time (" + now.toDecimal() + ") is after validBefore ("
                         + validBefore().toDecimal() + ")");
    }
    return std::nullopt;
}

std::optional<HttpError> Authorization::validate(const MachinePayConfig& config, const ResourceConfig& resource)
{
    // Check validAfter is less than or equal to current time
    // Check validBefore is greater than current time
    try
    {
        if (this->to() != config.network()->walletAddress())
        {
            return HttpError(ErrorType::ERR_BAD_REQUEST,
                             std::string(
                                 "Authorization payment destination address does not match configured destination address: ")
                             +
                             "authorization.to=" + to().toHex(PREFIX_0x) + ", configured.to=" + config.network()->
                             walletAddress()
                             .toHex(PREFIX_0x));
        }

        EIP3009Value price(resource.price());

        if (value() != price)
        {
            return HttpError(ErrorType::ERR_BAD_REQUEST,
                             std::string(
                                 "Payment value does not equal price of the resource (maxAmountRequired): ")
                             +
                             "authorization.value=" + value().toDecimal() +
                             ", resource.maxAmountRequired=" + price.toDecimal());
        }

        return checkValidityTime();
    }
    catch (const std::exception& e)
    {
        return HttpError(ErrorType::ERR_INTERNAL_SERVER_ERROR,
                         std::string("Authorization failed to validate") + e.what());
    }
    return std::nullopt; // no error
}