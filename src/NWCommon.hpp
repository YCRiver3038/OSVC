#ifndef NW_COMMON_H_INCLUDED
#define NW_COMMON_H_INCLUDED

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

#endif
