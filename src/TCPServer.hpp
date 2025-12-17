#ifndef TCPSERVER_H_INCLUDED
#define TCPSERVER_H_INCLUDED

#include <string>
#include <map>
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "errno.h"

#include "fcntl.h"
#include "unistd.h"
#include "netdb.h"
#include "sys/poll.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "netinet/ip.h"
#include <sys/socket.h>

#define TSRV_TIMEOUT_MSEC 1000
#define TSRV_ERR_TIMEOUT -1024
#define TSRV_ERR_CONN_CLOSED -1025
#define TSRV_ERR_GENERAL -2048

extern volatile bool servTerminate;

class TCPServer {
    private:
        struct sockaddr sAddr;
        struct addrinfo* destInfo = nullptr;
        struct addrinfo addrInfoHint = {};
        std::map<int32_t, int> cList;
        int32_t cListLength = 0;
        int sockFd = 0;
        struct pollfd aPollFd; // accept pollfd
        struct pollfd rPollFd; // recv poll fd
        struct pollfd sPollFd; // send poll fd
        bool srvBind = false;
#ifdef _GNU_SOURCE
        const short rPollFlag = POLLIN | POLLRDHUP | POLLHUP | POLLERR | POLLNVAL; // recv poll flags
        const short aPollFlag = POLLIN | POLLRDHUP | POLLHUP| POLLERR | POLLNVAL; // accept poll flags
        const short sPollFlag = POLLOUT | POLLHUP | POLLERR | POLLNVAL; // send poll flags
        const short acHupMask = 0|POLLRDHUP|POLLHUP;
        const short rdHupMask = 0|POLLRDHUP|POLLHUP;
#else
        const short rPollFlag = POLLIN | POLLERR | POLLNVAL; // recv poll flags
        const short aPollFlag = POLLIN  | POLLERR | POLLNVAL; // accept poll flags
        const short sPollFlag = POLLOUT | POLLHUP | POLLERR | POLLNVAL; // send poll flags
        const short acHupMask = 0|POLLHUP;
        const short rdHupMask = 0|POLLHUP;
#endif

    public:
        TCPServer(std::string bindAddr, std::string bindPort);
        ~TCPServer();
        bool binded();
        int32_t await();
        ssize_t sendTo(int32_t cID, uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(int32_t cID, uint8_t* rBuffer, uint32_t bufLength);
};

#endif
