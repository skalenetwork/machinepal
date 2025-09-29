#pragma once

#include <exception>
#include <string>

class BackendException : public std::exception {
public:
    explicit BackendException(const std::string& message)
        : message(message) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};
