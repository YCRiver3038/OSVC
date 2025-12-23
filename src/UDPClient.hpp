#ifndef UDPClient_H_INCLUDED
#define UDPClient_H_INCLUDED

#include "NWCommon.hpp"

#define UCLI_TIMEOUT_MSEC 1000
#define UCLI_TIMEOUT_SEC 1
#define UCLI_TIMEOUT_USEC 500000
#define UCLI_ERR_TIMEOUT -1024
#define UCLI_ERR_CONN_CLOSED -1025
#define UCLI_ERR_GENERAL -2048

extern volatile bool ucliTerminate;

class UDPClient {
    private:
#if defined(_WIN32) || defined(_WIN64)
        WORD wVersionRequested;
        WSADATA wsaData;
        int wInitStatus = 0;
#endif
        struct sockaddr sAddr;
        struct addrinfo* destInfo = nullptr;
        struct addrinfo addrInfoHint = {};
        int32_t cListLength = 0;
        int sockFd = 0;
        bool connected = false;

    public:
        UDPClient(std::string conAddr, std::string conPort);
        ~UDPClient();
        bool isConnected();
        ssize_t sendTo(uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(uint8_t* rBuffer, uint32_t bufLength);
        ssize_t retryConnect();
};

#endif
