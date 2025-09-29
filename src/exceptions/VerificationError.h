#pragma once

#include <exception>
#include <string>


class VerificationError : public std::exception {
public:
    explicit VerificationError(const std::string& _message)
        : message(_message) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};
