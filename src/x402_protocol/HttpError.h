#pragma once


enum ErrorType {
    ERR_BAD_REQUEST = 400,
    ERR_INTERNAL_SERVER_ERROR = 500,
    ERR_BAD_GATEWAY = 502
};

class HttpError {
    ErrorType type_;
    std::string message_;

public:
    HttpError(ErrorType type, const std::string &message)
        : type_(type),
          message_(message) {
    }

    [[nodiscard]] ErrorType type() const {
        return type_;
    }

    [[nodiscard]] std::string message() const {
        return message_;
    }
};

