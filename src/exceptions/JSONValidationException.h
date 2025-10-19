#pragma once

#include <exception>
#include <string>
#include <nlohmann/json.hpp>

class JSONValidationException : public std::exception {
public:
    JSONValidationException(const std::string& message, const nlohmann::json& j)
        : message_(message), json_(j) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const nlohmann::json& getJson() const {
        return json_;
    }

private:
    std::string message_;
    nlohmann::json json_;
};

