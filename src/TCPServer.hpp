#ifndef TCPSERVER_H_INCLUDED
#define TCPSERVER_H_INCLUDED

#include <string>
#include <map>
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "errno.h"

#if defined(_WIN32) || defined(_WIN64)
//ref https://learn.microsoft.com/ja-jp/windows/win32/api/winsock/nf-winsock-wsastartup
//
//    WORD wVersionRequested;
//    WSADATA wsaData;
//    int err;
//
///* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
//    wVersionRequested = MAKEWORD(2, 2);

//    err = WSAStartup(wVersionRequested, &wsaData);
//    if (err != 0) {
//        /* Tell the user that we could not find a usable */
//        /* Winsock DLL.                                  */
//        printf("WSAStartup failed with error: %d\n", err);
//        return 1;
//    }

#include <winsock2.h>
#include "winsock.h"
#include <windows.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#define poll(fdArray, fds, timeout) WSAPoll(fdArray, fds, timeout)
#define close(close_fd) closesocket(close_fd)
#else
#include <sys/select.h>
#include "fcntl.h"
#include "unistd.h"
#include "netdb.h"
#include "sys/poll.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "netinet/ip.h"
#include <sys/socket.h>
#endif

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
        bool bound();
        int32_t await();
        ssize_t sendTo(int32_t cID, uint8_t* sBuffer, uint32_t bufLength);
        ssize_t recvFrom(int32_t cID, uint8_t* rBuffer, uint32_t bufLength);
};

#endif
