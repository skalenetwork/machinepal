#pragma once

#include "BackendException.h"
#include <string>

class UnknownErrorException : public BackendException {
public:
    explicit UnknownErrorException(const std::string& message)
        : BackendException(message) {}
};
