#pragma once

#include "BackendError.h"

class BackendCurlError : public BackendError {

public:
    BackendCurlError(uint64_t error, const std::string &message)
        : error_(error),
          message_(message) {
    }

    uint64_t getError() const {
        return error_;
    }

    const std::string& getMessage() const {
        return message_;
    }

    virtual ~BackendCurlError() = default;

private:
    uint64_t error_;
    std::string message_;
};