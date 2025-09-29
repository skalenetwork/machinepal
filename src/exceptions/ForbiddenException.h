#pragma once

#include <exception>
#include <string>

class ForbiddenException : public std::exception {
public:
    explicit ForbiddenException(const std::string& _message)
        : message_(_message) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

