#pragma once


#define CHECK_STATE_JSON(_EXPRESSION_, __MSG__, __JSON__) \
if (!(_EXPRESSION_)) { \
auto __msg__ = std::string("Check failed:") + __MSG__ + "\n" + j.dump(4); \
throw std::logic_error(__msg__ + "(): " + std::string(__MSG__)); \
}



#pragma once


#include "MachinePayConfig.h"
#include "nlohmann/json.hpp"
#include <mutex>

class MachinePayConfig;


class JsonUtils {
public:



    static bool getBoolWithDefault(
            const nlohmann::json &j, const std::string &key, bool defaultValue);

    static uint16_t getUint16WithDefault(const nlohmann::json &j, const std::string &key,
                                                         uint16_t defaultValue);

    static std::string getStringWithDefault(
        const nlohmann::json &j, const std::string &key, const std::string &defaultValue);


    static std::string findPath(const json& root, const json& target, const std::string& current = "") {
        if (&root == &target) {
            return current.empty() ? "/" : current;
        }

        if (root.is_object()) {
            for (auto it = root.begin(); it != root.end(); ++it) {
                std::string child_path = current + "/" + it.key();
                std::string p = findPath(it.value(), target, child_path);
                if (!p.empty()) return p;
            }
        } else if (root.is_array()) {
            for (size_t i = 0; i < root.size(); ++i) {
                std::string child_path = current + "/" + std::to_string(i);
                std::string p = findPath(root[i], target, child_path);
                if (!p.empty()) return p;
            }
        }

        return "";
    }


    static bool asBool(const std::string& s) {
        return s == "1" || s == "true" || s == "TRUE" || s == "yes" || s == "on";
    };


    // Helper to get a string from a json object with a default value
    std::string ConfigLoader::getStringWithDefault(const nlohmann::json &j, const std::string &key,
                                                             const std::string &defaultValue) {
        if (j.contains(key) && !j.at(key).is_null()) {
            return j.at(key).get<std::string>();
        }
        return defaultValue;
    }

    bool JsonUtils::getBoolWithDefault(const nlohmann::json &j, const std::string &key,
                                                             bool defaultValue) {
        if (j.contains(key) && !j.at(key).is_null()) {
            return j.at(key).get<bool>();
        }
        return defaultValue;
    }



    uint16_t ConfigLoader::getUint16WithDefault(const nlohmann::json &j, const std::string &key,
                                                         uint16_t defaultValue) {
        if (j.contains(key) && !j.at(key).is_null()) {
            auto value = j.at(key).get<int>();
            if (value <= 0 || value > 65535) {
                throw std::out_of_range("Value for key '" + key + "' is out of range for uint16_t: " + std::to_string(value));
            }
            return static_cast<uint16_t>(value);
        }
        return defaultValue;
    }


};