#include "TCPServer.hpp"

volatile bool servTerminate = false;
const int timeoutCountMax = 10;

bool TCPServer::bound() {
    return srvBind;
}

TCPServer::TCPServer(std::string bindAddr, std::string bindPort) {
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

    rPollFd.events = rPollFlag;
    aPollFd.events = aPollFlag;
    sPollFd.events = sPollFlag;

    addrInfoHint.ai_protocol = IPPROTO_TCP;
    addrInfoHint.ai_family = PF_UNSPEC;
    addrInfoHint.ai_socktype = SOCK_STREAM;
    addrInfoHint.ai_flags = addrInfoHint.ai_flags | AI_PASSIVE;
    getaddrinfo(bindAddr.c_str(), bindPort.c_str(), &addrInfoHint, &destInfo);

    if (destInfo == nullptr) {
        return;
    }
    for (struct addrinfo* diRef = destInfo; diRef != nullptr; diRef = diRef->ai_next) {
        if (servTerminate) {
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
                int listen_status = 0;
                listen_status = listen(sockFd, 8);
                retErrno = errno;
                if (listen_status != 0) {
                    return;
                }
#if defined(_WIN32) || defined(_WIN64)
                u_long ioctlret = 1;
                ioctlsocket(sockFd, FIONBIO, &ioctlret);
#else
                fcntl(sockFd, F_SETFL, O_NONBLOCK);
#endif
                aPollFd.fd = sockFd;
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

TCPServer::~TCPServer() {
    for (auto client = cList.begin(); client != cList.end(); client++) {
        close(client->second);
    }
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

/*
    cAccept() returns cID of accepted client if successed
*/
int32_t TCPServer::await() {
    fd_set aFdSet;
    fd_set eFdSet;
    FD_ZERO(&aFdSet);
    FD_ZERO(&eFdSet);

    int acceptedFd = 0;
    int acceptErrno = 0;
    int selectResult = 0;
    int selectErrno = 0;
    struct sockaddr_storage peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    char hostName[256] = {};
    char svcName[32] = {};
    
    struct timeval timeoutVal;
    FD_SET(aSockFd, &aFdSet);
    FD_SET(aSockFd, &eFdSet);
    timeoutVal.tv_sec = 0;
    timeoutVal.tv_usec = TSRV_TIMEOUT_USEC;

    selectResult = select(aSockFd+1, &aFdSet, nullptr, &eFdSet, &timeoutVal);
    //selectResult = poll(&aPollFd, 1, TSRV_TIMEOUT_MSEC);
    selectErrno = (int)errno;
    if (selectResult == -1) {
        return (int)(selectErrno * -1);
    }
    if (selectResult == 0) {
        return (int)TSRV_ERR_TIMEOUT;
    }
    //if (FD_ISSET(aSockFd, &eFdSet)) { //if (aPollFd.revents & acHupMask) {
    //    return (int)TSRV_ERR_CONN_CLOSED;
    //}
    if (FD_ISSET(aSockFd, &eFdSet)) { //if (aPollFd.revents & (0|POLLERR|POLLNVAL)) {
        return (int)TSRV_ERR_GENERAL;
    }
#ifdef _GNU_SOURCE
    acceptedFd = accept4(sockFd, (struct sockaddr*)&peerAddr, &peerAddrLen, 0|SOCK_NONBLOCK|SOCK_CLOEXEC);
#else
    acceptedFd = accept(sockFd, (struct sockaddr*)&peerAddr, &peerAddrLen);
    if (acceptedFd > 0) {
#if defined(_WIN32) || defined(_WIN64)
        u_long ioctlret = 1;
        ioctlsocket(acceptedFd, FIONBIO, &ioctlret);
#else
        fcntl(acceptedFd, F_SETFL, O_NONBLOCK); // ノンブロッキング化
#endif
//        int opt = 1;
//        setsockopt(acceptedFd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
    }
#endif
    acceptErrno = errno;
    if (acceptedFd > 0) {
        //connect(acceptedFd, &peerAddr, peerAddrLen);
        //perror("debug");
        getnameinfo((struct sockaddr*)&peerAddr, peerAddrLen, hostName, 256, svcName, 32, 0|NI_NUMERICHOST|NI_NUMERICSERV);
        printf("Accepted [%s]:%s\n", hostName, svcName);
        cList.emplace(cListLength, acceptedFd);
        cListLength++;
        return cListLength - 1;
    }
    return -1 * acceptErrno;
}

ssize_t TCPServer::sendTo(int32_t cID, uint8_t* sBuffer, uint32_t bufLength) {
    if (sBuffer == nullptr) {
        return (ssize_t)TSRV_ERR_GENERAL;
    }
    int fdNum = 0;
    fd_set sFdSet;
    fd_set eFdSet;
    FD_ZERO(&sFdSet);
    FD_ZERO(&eFdSet);

    int sendErrno = 0;
    ssize_t sendHeadIndex= 0;
    ssize_t sendRemain = 0;
    ssize_t sentLength = 0;
    int timeoutCount = 0;
    int selectResult = 0;
    ssize_t selectErrno = 0;
    struct pollfd stPoll = {};

    //int selResult = 0;
    struct timeval timeoutVal;

    sendHeadIndex = 0;
    sendRemain = bufLength;
    try {
        fdNum = cList.at(cID);
        FD_SET(fdNum, &sFdSet);
        FD_SET(fdNum, &eFdSet);
        stPoll.events = sPollFlag;
        stPoll.fd = fdNum;
    } catch (std::exception& e) {
        printf("Send error: %s\n", e.what());
        return TSRV_ERR_GENERAL;
    }
    while ((sendRemain > 0) && !servTerminate) {
        timeoutVal.tv_sec = 0;
        timeoutVal.tv_usec = TSRV_TIMEOUT_USEC;
        FD_SET(fdNum, &sFdSet);
        FD_SET(fdNum, &eFdSet);

        //selectResult = poll(&stPoll, 1, TSRV_TIMEOUT_MSEC);
        selectResult = select(fdNum+1, nullptr, &sFdSet, &eFdSet, &timeoutVal);
        selectErrno = errno;
        if (selectResult == -1) {
            try {
                close(fdNum);
                cList.erase(cID);
            } catch (std::exception& e) {
                printf("Client erased: %s\n", e.what());
            }
            FD_CLR(fdNum, &sFdSet);
            FD_CLR(fdNum, &eFdSet);
            return (ssize_t)(selectErrno * -1);
        }
        if (selectResult == 0) {
            timeoutCount++;
            if (timeoutCount > timeoutCountMax) {
                FD_CLR(fdNum, &sFdSet);
                FD_CLR(fdNum, &eFdSet);
                return TSRV_ERR_TIMEOUT;
            }
        }
        if (selectResult > 0) {
            printf("Sending to %d...\n", fdNum);
            if (sendHeadIndex >= bufLength) {
                break;
            }
            if (FD_ISSET(fdNum, &sFdSet)) { //(stPoll.revents & (0|POLLOUT)) {
#if defined(_WIN32) || defined(_WIN64)
                sentLength = send(fdNum, (char*)&(sBuffer[sendHeadIndex]), sendRemain, 0);
#else
                sentLength = send(fdNum, &(sBuffer[sendHeadIndex]), sendRemain, 0);
#endif
                sendErrno = errno;
                if (sentLength > 0) {
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
                            FD_CLR(fdNum, &eFdSet);
                            return (sendErrno * -1);
                    }
                }
            }
            if (FD_ISSET(fdNum, &eFdSet)) { //else {
                printf("Select error.\n");
                try {
                    close(fdNum);
                    cList.erase(cID);
                    FD_CLR(fdNum, &sFdSet);
                    FD_CLR(fdNum, &eFdSet);
                } catch (std::exception& e) {
                    printf("Client erased: %s\n", e.what());
                }
                return (ssize_t)TSRV_ERR_CONN_CLOSED;
                //perror("debug");
                /*
                if (stPoll.revents & (0|POLLHUP)) {
                    try {
                        close(stPoll.fd);
                        cList.erase(cID);
                    } catch (std::exception& e) {
                        printf("Client erased: %s\n", e.what());
                    }
                    return (ssize_t)TSRV_ERR_CONN_CLOSED;
                }
                if (stPoll.revents & (0|POLLERR|POLLNVAL)) {
                    try {
                        close(stPoll.fd);
                        cList.erase(cID);
                    } catch (std::exception& e) {
                        printf("Client erased: %s\n", e.what());
                    }
                    return TSRV_ERR_GENERAL;
                }*/
            }
        }
    }
    return sendHeadIndex;
}

ssize_t TCPServer::recvFrom(int32_t cID, uint8_t* rBuffer, uint32_t bufLength) {
    ssize_t rfBytesLength = 0;
    ssize_t rfselectErrno = 0;
    int rfPollRes = 0;
    int rfErrno = 0;
    struct pollfd rfPoll = {};
    try {
        rfPoll.events = rPollFlag;
        rfPoll.fd = cList.at(cID);
    } catch (std::exception& e) {
        printf("Recv error: %s\n", e.what());
        return TSRV_ERR_GENERAL;
    }

    if (rBuffer == nullptr) {
        return TSRV_ERR_GENERAL;
    }

    rfPollRes = poll(&rfPoll, 1, TSRV_TIMEOUT_MSEC);
    rfselectErrno = (ssize_t)errno;
    if (rfPollRes == -1) {
        close(rfPoll.fd);
        return (ssize_t)(rfselectErrno * -1);
    }
    if (rfPollRes == 0) {
        return (ssize_t)TSRV_ERR_TIMEOUT;
    }
    if (rfPoll.revents & rdHupMask) {
        close(rfPoll.fd);
        return (ssize_t)TSRV_ERR_CONN_CLOSED;
    }
    if (rfPoll.revents & (0|POLLERR|POLLNVAL)) {
        close(rfPoll.fd);
        return (ssize_t)TSRV_ERR_GENERAL;
    }
    if (rfPoll.revents & 0|POLLIN) {
#if defined(_WIN32) || defined(_WIN64)
        rfBytesLength = recvfrom(rfPoll.fd, (char*)rBuffer, bufLength, 0, nullptr, 0);
#else
        rfBytesLength = recvfrom(rfPoll.fd, rBuffer, bufLength, 0, nullptr, 0);
#endif
        rfErrno = errno;
        if (rfBytesLength < 0) {
            close(rfPoll.fd);
            return -1*rfErrno;
        }
        if (rfBytesLength == 0) {
            return 0;
        }
        return rfBytesLength;
    }
    close(rfPoll.fd);
    return TSRV_ERR_GENERAL;
}