#pragma once
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>
#include <map>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <regex>
#include <set>
#include <nlohmann/json_fwd.hpp>
#include "boost/url/decode_view.hpp"
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <array>
#include <cstdlib>

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
    constexpr bool exceptions_enabled = true;
#else
constexpr bool exceptions_enabled = false;
#endif

static_assert(exceptions_enabled, "Exceptions must be enabled!");



#define CHECK_STATE(_EXPRESSION_) \
    if (!(_EXPRESSION_)) { \
        auto __msg__ = std::string("Check failed::") + #_EXPRESSION_ + " " + std::string(__FILE__) + \
                       ":" + std::to_string(__LINE__) + " " + __FUNCTION__; \
        throw std::logic_error(__msg__ + "()"); \
    }



#define CHECK_STATE2(_EXPRESSION_, __MSG__) \
    if (!(_EXPRESSION_)) { \
        auto __msg__ = std::string("Check failed::") + #_EXPRESSION_ + " " + std::string(__FILE__) + \
                       ":" + std::to_string(__LINE__) + " " + __FUNCTION__; \
        throw std::logic_error(__msg__ + "(): " + std::string(__MSG__)); \
    }




inline std::string stripSpaces(std::string _s) {
    _s.erase(std::remove_if(_s.begin(), _s.end(), ::isspace), _s.end());
    return _s;
}

inline void printNestedException(const std::exception& e, int level = 0) {
    spdlog::error("{}Exception: {}", std::string(level, '*'), e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& nested) {
        printNestedException(nested, level + 2);
    } catch (...) {
        spdlog::error("{}Non-std::exception nested", std::string(level + 2, '*'));
    }
}

#include <glog/logging.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <stdexcept>


#define RETHROW_NESTED \
do { std::throw_with_nested(std::runtime_error(std::string(__FILE__) + \
":" + std::to_string(__LINE__) + " " + std::string(__PRETTY_FUNCTION__))); \
} while(0)

#define RETHROW_NESTED2(__MSG__) \
do { std::throw_with_nested(std::runtime_error(std::string(__FILE__) + ":" \
+ std::to_string(__LINE__) + " " + std::string(__PRETTY_FUNCTION__) + ":" + std::string(__MSG__))); \
} while(0)


template<typename T>
using ptr = std::shared_ptr<T>;

using Hash = std::array<uint8_t, 32>;;

using namespace std;


