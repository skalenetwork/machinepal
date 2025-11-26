#pragma once
#include "BackendError.h"

class BackendHttpError : public BackendError {

public:

    uint64_t getError() const {
        return error_;
    }

    const std::string& getMessage() const {
        return message_;
    }

    virtual ~BackendHttpError() = default;

private:
    uint64_t error_;
    std::string message_;
};
