#pragma once

class IResponseSender {
public:
    virtual ~IResponseSender() = default;
    virtual void sendResponse( const std::pair< uint16_t, std::string >& statusAndMessage,
        const std::vector< std::pair< std::string, std::string > >& headers,
        const std::string& body = "" ) = 0;
};