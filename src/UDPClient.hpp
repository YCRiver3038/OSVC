#ifndef UDPClient_H_INCLUDED
#define UDPClient_H_INCLUDED


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
