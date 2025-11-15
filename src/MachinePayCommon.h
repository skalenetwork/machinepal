#pragma once

// Third-party / external libraries
#include <spdlog/spdlog.h>
#include <glog/logging.h>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

// Boost libraries
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/regex.hpp>
#include <boost/url/decode_view.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/url/error.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

// Standard library headers
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <span>
#include <set>
#include <variant>
#include <csignal>
#include <curl/curl.h>
#include <folly/json.h>
#include <limits>   // for overflow check
#include <random>   // added for nonce
#include <fstream>
#include <mutex>
#include <cstddef>
#include <utility>




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


inline std::string stripSpaces(std::string _s)
{
    _s.erase(std::remove_if(_s.begin(), _s.end(), ::isspace), _s.end());
    return _s;
}

inline void printNestedException(const std::exception& e, int level = 0)
{
    spdlog::error("{}Exception: {}", std::string(level, '*'), e.what());
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nested)
    {
        printNestedException(nested, level + 2);
    }
    catch (...)
    {
        spdlog::error("{}Non-std::exception nested", std::string(level + 2, '*'));
    }
}


#define RETHROW_NESTED \
do { std::throw_with_nested(std::runtime_error(std::string(__FILE__) + \
":" + std::to_string(__LINE__) + " " + std::string(__PRETTY_FUNCTION__))); \
} while(0)

#define RETHROW_NESTED2(__MSG__) \
do { std::throw_with_nested(std::runtime_error(std::string(__FILE__) + ":" \
+ std::to_string(__LINE__) + " " + std::string(__PRETTY_FUNCTION__) + ":" + std::string(__MSG__))); \
} while(0)


template <typename T>
using ptr = std::shared_ptr<T>;

using Hash = std::array<uint8_t, 32>;

using u256 = boost::multiprecision::uint256_t;

using json = nlohmann::json;

using namespace std;

enum Prefix
{
    PREFIX_NONE,
    PREFIX_0x
};