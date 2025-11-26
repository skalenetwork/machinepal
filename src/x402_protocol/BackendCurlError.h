#pragma once


class BackendCurlError {
public:
    uint64_t getError() const {
        return error_;
    }

    const std::string& getMessage() const {
        return message_;
    }

private:
    uint64_t error_;
    std::string message_;
};