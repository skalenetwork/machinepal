#pragma once

#include <exception>
#include <string>

class UnknownErrorException : public std::exception {
public:
    explicit UnknownErrorException(const std::string& _message)
        : message_(_message) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

