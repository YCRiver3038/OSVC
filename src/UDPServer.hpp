#ifndef UDPSERVER_H_INCLUDED
#define UDPSERVER_H_INCLUDED

#include "NWCommon.hpp"

#define USRV_TIMEOUT_MSEC 1000
#define USRV_TIMEOUT_SEC 1
#define USRV_TIMEOUT_USEC 500000
#define USRV_ERR_TIMEOUT -1024
#define USRV_ERR_CONN_CLOSED -1025
#define USRV_ERR_GENERAL -2048

extern volatile bool uServTerminate;

class UDPServer {
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
        UDPServer(std::string bindAddr, std::string bindPort);
        ~UDPServer();
        bool bound();
        //int32_t await();
        ssize_t sendTo(uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(uint8_t* rBuffer, uint32_t bufLength);
};

#endif
