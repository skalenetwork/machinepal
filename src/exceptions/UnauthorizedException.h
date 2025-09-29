#pragma once

#include <exception>
#include <string>

class UnauthorizedException : public std::exception {
public:
    explicit UnauthorizedException(const std::string& _message)
        : message(_message) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};
