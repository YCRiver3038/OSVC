#ifndef TCPSERVER_H_INCLUDED
#define TCPSERVER_H_INCLUDED

#include "NWCommon.hpp"

#define TSRV_TIMEOUT_MSEC 1000
#define TSRV_TIMEOUT_SEC 1
#define TSRV_TIMEOUT_USEC 500000
#define TSRV_ERR_TIMEOUT -1024
#define TSRV_ERR_CONN_CLOSED -1025
#define TSRV_ERR_GENERAL -2048

extern volatile bool servTerminate;

class TCPServer {
    private:
#if defined(_WIN32) || defined(_WIN64)
        WORD wVersionRequested;
        WSADATA wsaData;
        int wInitStatus = 0;
#endif
        struct sockaddr sAddr;
        struct addrinfo* destInfo = nullptr;
        struct addrinfo addrInfoHint = {};
        std::map<int32_t, int> cList;
        int32_t cListLength = 0;
        int sockFd = 0;
        int aSockFd = 0;
        bool srvBind = false;

    public:
        TCPServer(std::string bindAddr, std::string bindPort);
        ~TCPServer();
        bool bound();
        int32_t await();
        ssize_t sendTo(int32_t cID, uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(int32_t cID, uint8_t* rBuffer, uint32_t bufLength);
};

#endif
