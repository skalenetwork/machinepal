#include "MachinePayCommon.h"
#include "Authorization.h"
#include "x402_protocol/HttpError.h"
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <cctype>
#include <sstream>

static uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    throw std::invalid_argument("Invalid hex character");
}

static std::string toLowerHex(uint8_t v) {
    const char* hex = "0123456789abcdef";
    std::string s;
    s.push_back(hex[(v >> 4) & 0xF]);
    s.push_back(hex[v & 0xF]);
    return s;
}

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
    from_ = parseHexAddress(fromStr);
    to_ = parseHexAddress(toStr);
    fromHex_ = addressToHex(from_); // normalized 0x lowercase
    toHex_ = addressToHex(to_);
}

const std::string &Authorization::value() const { return value_; }
const std::string &Authorization::validAfter() const { return validAfter_; }
const std::string &Authorization::validBefore() const { return validBefore_; }
const std::string &Authorization::nonce() const { return nonce_; }

// Return hex strings (updated to match header)
const std::string& Authorization::from() const { return fromHex_; }
const std::string& Authorization::to() const { return toHex_; }

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

Address Authorization::parseHexAddress(const std::string& hex) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() != 40) {
        throw std::invalid_argument("Address hex must be 40 characters (20 bytes)");
    }
    Address addr{};
    for (size_t i = 0; i < 20; ++i) {
        uint8_t high = hexNibble(s[2*i]);
        uint8_t low = hexNibble(s[2*i + 1]);
        addr[i] = static_cast<uint8_t>((high << 4) | low);
    }
    return addr;
}

std::string Authorization::addressToHex(const Address& addr) {
    std::string out = "0x";
    out.reserve(2 + 40);
    for (uint8_t b : addr) {
        out += toLowerHex(b);
    }
    return out;
}
