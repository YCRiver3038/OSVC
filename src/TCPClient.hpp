#ifndef TCPCLIENT_H_INCLUDED
#define TCPCLIENT_H_INCLUDED

#include "NWCommon.hpp"

#define TCLI_TIMEOUT_MSEC 1000
#define TCLI_TIMEOUT_SEC 1
#define TCLI_TIMEOUT_USEC 500000
#define TCLI_ERR_TIMEOUT -1024
#define TCLI_ERR_CONN_CLOSED -1025
#define TCLI_ERR_GENERAL -2048

extern volatile bool tcliTerminate;

class TCPClient {
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
        TCPClient(std::string conAddr, std::string conPort);
        ~TCPClient();
        bool isConnected();
        ssize_t sendTo(uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(uint8_t* rBuffer, uint32_t bufLength);
};

#endif
