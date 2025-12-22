#include "UDPServer.hpp"

volatile bool uServTerminate = false;
const int timeoutCountMax = 10;

bool UDPServer::bound() {
    return srvBind;
}

UDPServer::UDPServer(std::string bindAddr, std::string bindPort) {
    int retErrno = 0;
    int bindStatus = 0;
    char hostName[256] = {};
    char svcName[32] = {};

#if defined(_WIN32) || defined(_WIN64)
    char sockOptValue = 1;
    wVersionRequested = MAKEWORD(2, 2);
    wInitStatus = WSAStartup(wVersionRequested, &wsaData);
    if (wInitStatus != 0) {
        printf("WSASetup error: %d\n", wInitStatus);
        return;
    }
#else
    int sockOptValue = 1;
#endif

    addrInfoHint.ai_protocol = IPPROTO_UDP;
    addrInfoHint.ai_family = PF_UNSPEC;
    addrInfoHint.ai_socktype = SOCK_DGRAM;
    addrInfoHint.ai_flags = addrInfoHint.ai_flags | AI_PASSIVE;
    getaddrinfo(bindAddr.c_str(), bindPort.c_str(), &addrInfoHint, &destInfo);

    if (destInfo == nullptr) {
        return;
    }
    for (struct addrinfo* diRef = destInfo; diRef != nullptr; diRef = diRef->ai_next) {
        if (uServTerminate) {
            break;
        }
        sockFd = socket(diRef->ai_family, diRef->ai_socktype, 0);
        retErrno = errno;
        if (sockFd != -1) {
            setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &sockOptValue, sizeof(sockOptValue));
            bindStatus = bind(sockFd, diRef->ai_addr, diRef->ai_addrlen);
            retErrno = errno;
            if (bindStatus == 0) {
                srvBind = true;
#if defined(_WIN32) || defined(_WIN64)
                u_long ioctlret = 1;
                ioctlsocket(sockFd, FIONBIO, &ioctlret);
#else
                int oldFlag = 0;
                int fcerr = 0;
                oldFlag = fcntl(sockFd, F_GETFL);
                fcerr = errno;
                if (oldFlag != -1) {
                    fcntl(sockFd, F_SETFL, oldFlag|O_NONBLOCK); // ノンブロッキング化
                    fcerr = errno;
                } else {
                    printf("UDPServer: fcntl returned -1: cannot set nonblock option\n");
                }
#endif
#if defined(_WIN32) || defined(_WIN64)
                char opt = 1;
#else
                int opt = 1;
#endif
#ifdef SO_NOSIGPIPE
                setsockopt(sockFd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif
                setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                aSockFd = sockFd;
                getnameinfo(diRef->ai_addr, diRef->ai_addrlen, hostName, 256, svcName, 32, 0|NI_NUMERICHOST|NI_NUMERICSERV);
                printf("Bound on [%s]:%s, ", hostName, svcName);
                printf("Proto: %s, Type: %s, Family: %s\n", (diRef->ai_protocol == IPPROTO_TCP) ? "TCP" :
                                                            (diRef->ai_protocol == IPPROTO_UDP) ? "UDP" :
                                                            "Other"
                                                          , (diRef->ai_socktype == SOCK_STREAM) ? "STREAM" :
                                                            (diRef->ai_socktype == SOCK_DGRAM)  ? "DGRAM" :
                                                            "Other"
                                                          , (diRef->ai_family == PF_INET) ? "IPv4" :
                                                            (diRef->ai_family == PF_INET6) ? "IPv6" : "Other");
                return;
            }
        }
    }
    return;
}

UDPServer::~UDPServer() {
    /*for (auto client = cList.begin(); client != cList.end(); client++) {
        close(client->second);
    }*/
    if (srvBind) {
        close(sockFd);
    }
    if (destInfo) {
        freeaddrinfo(destInfo);
    }
#if defined(_WIN32) || defined(_WIN64)
    if (wInitStatus == 0) {
        WSACleanup();
    }
#endif
}

ssize_t UDPServer::sendTo(uint8_t* sBuffer, uint32_t bufLength) {
    if (sBuffer == nullptr) {
        return (ssize_t)USRV_ERR_GENERAL;
    }
    int fdNum = 0;
    fd_set sFdSet;
    FD_ZERO(&sFdSet);

    int sendErrno = 0;
    ssize_t sendHeadIndex= 0;
    ssize_t sendRemain = 0;
    ssize_t sentLength = 0;
    int timeoutCount = 0;
    int selectResult = 0;
    ssize_t selectErrno = 0;

    struct timeval timeoutVal;

    sendHeadIndex = 0;
    sendRemain = bufLength;
    try {
        fdNum = sockFd;
        FD_SET(fdNum, &sFdSet);
    } catch (std::exception& e) {
        printf("Send error: no corresponding ID ( %s )\n", e.what());
        return USRV_ERR_GENERAL;
    }
    while ((sendRemain > 0) && !uServTerminate) {
        timeoutVal.tv_sec = 0;
        timeoutVal.tv_usec = USRV_TIMEOUT_USEC;
        FD_SET(fdNum, &sFdSet);
        selectResult = select(fdNum+1, nullptr, &sFdSet, nullptr, &timeoutVal);
        selectErrno = errno;
        if (selectResult == -1) {
            FD_CLR(fdNum, &sFdSet);
            try {
                close(fdNum);
            } catch (std::exception& e) {
                printf("Client erased: %s\n", e.what());
            }
            printf("send select error: %zd ( %s )...\n", selectErrno, strerror(selectErrno));
            return (ssize_t)(selectErrno * -1);
        }
        if (selectResult == 0) {
            timeoutCount++;
            if (timeoutCount > timeoutCountMax) {
                FD_CLR(fdNum, &sFdSet);
                return USRV_ERR_TIMEOUT;
            }
        }
        printf("Sending to %d...\n", fdNum);
        if (sendHeadIndex >= bufLength) {
            break;
        }
        if (FD_ISSET(fdNum, &sFdSet)) {
#if defined(_WIN32) || defined(_WIN64)
            sentLength = send(fdNum, (char*)&(sBuffer[sendHeadIndex]), sendRemain, 0);
#else
            sentLength = send(fdNum, &(sBuffer[sendHeadIndex]), sendRemain, 0);
#endif
            sendErrno = errno;
            if (sentLength > 0) {
                printf("OK\n");
                sendHeadIndex += sentLength;
                sendRemain -= sentLength;
            } else {
                switch(sendErrno){
                    case EAGAIN:
                        continue;
                    default:
                        printf("Send returned %zd\n", sentLength);
                        printf("Send errno: %d (%s) \n", sendErrno, strerror(sendErrno));
                        FD_CLR(fdNum, &sFdSet);
                        return (sendErrno * -1);
                }
            }
        }
        printf("Send remain: %zd\n", sendRemain);
    }
    return sendHeadIndex;
}

ssize_t UDPServer::recvFrom(uint8_t* rBuffer, uint32_t bufLength) {
    if (!rBuffer) {
        return USRV_ERR_GENERAL;
    }

    int fdNum = 0;
    fd_set rFdSet;
    FD_ZERO(&rFdSet);

    struct timeval timeoutVal;

    ssize_t rfBytesLength = 0;
    ssize_t rfselectErrno = 0;
    int rfSelectRes = 0;
    int rfErrno = 0;

    try {
        fdNum = sockFd;
    } catch (std::exception& e) {
        printf("Recv error: %s\n", e.what());
        return USRV_ERR_GENERAL;
    }

    timeoutVal.tv_sec = 0;
    timeoutVal.tv_usec = USRV_TIMEOUT_USEC;
    FD_SET(fdNum, &rFdSet);
    rfSelectRes = select(fdNum+1, &rFdSet, nullptr, nullptr, &timeoutVal);
    rfselectErrno = (ssize_t)errno;
    if (rfSelectRes == -1) {
        perror("debug");
        printf("recv error: %d ( %s )...\n", fdNum, strerror(rfselectErrno));
        FD_CLR(fdNum, &rFdSet);
        close(fdNum);
        return (ssize_t)(rfselectErrno * -1);
    }
    if (rfSelectRes == 0) {
        FD_CLR(fdNum, &rFdSet);
        return (ssize_t)USRV_ERR_TIMEOUT;
    }
    if (FD_ISSET(fdNum, &rFdSet)) {
#if defined(_WIN32) || defined(_WIN64)
        rfBytesLength = recv(fdNum, (char*)rBuffer, bufLength, 0);
#else
        rfBytesLength = recv(fdNum, rBuffer, bufLength, 0);
#endif
        rfErrno = errno;
        if (rfBytesLength < 0) {
            FD_CLR(fdNum, &rFdSet);
            close(fdNum);
            return -1*rfErrno;
        }
        return rfBytesLength;
    }
    close(fdNum);
    return USRV_ERR_GENERAL;
}