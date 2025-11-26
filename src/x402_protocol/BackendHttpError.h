//
// Created by kladko on 11/26/25.
//

#ifndef MACHINEPAY_BACKENDHTTPERROR_H
#define MACHINEPAY_BACKENDHTTPERROR_H


class BackendHttpError {
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


#endif //MACHINEPAY_BACKENDHTTPERROR_H