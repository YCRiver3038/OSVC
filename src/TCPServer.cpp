#include "TCPServer.hpp"

volatile bool servTerminate = false;
const int timeoutCountMax = 10;

bool TCPServer::binded() {
    return srvBind;
}

TCPServer::TCPServer(std::string bindAddr, std::string bindPort) {
    int retErrno = 0;
    int bindStatus = 0;
    char hostName[256] = {};
    char svcName[32] = {};

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
        sockFd = socket(diRef->ai_family, diRef->ai_socktype, diRef->ai_protocol);
        retErrno = errno;
        if (sockFd != -1) {
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
                fcntl(sockFd, F_SETFL, O_NONBLOCK);
                aPollFd.fd = sockFd;
                getnameinfo(diRef->ai_addr, diRef->ai_addrlen, hostName, 256, svcName, 32, 0|NI_NUMERICHOST|NI_NUMERICSERV);
                printf("Bound on [%s]:%s, ", hostName, svcName);
                printf("Proto: %s, Type: %s, Family: %s\n", (diRef->ai_protocol == IPPROTO_TCP) ? "TCP" : "Other"
                                                          , (diRef->ai_socktype == SOCK_STREAM) ? "STREAM" : "Other"
                                                          , (diRef->ai_family == PF_INET) ? "IPv4" : "Other" );
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
}

/*
    cAccept() returns cID of accepted client if successed
*/
int32_t TCPServer::await() {
    int acceptedFd = 0;
    int acceptErrno = 0;
    int pollResult = 0;
    int pollErrno = 0;
    struct sockaddr peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    char hostName[256] = {};
    char svcName[32] = {};

    pollResult = poll(&aPollFd, 1, TSRV_TIMEOUT_MSEC);
    pollErrno = (int)errno;
    if (pollResult == -1) {
        return (int)(pollErrno * -1);
    }
    if (pollResult == 0) {
        return (int)TSRV_ERR_TIMEOUT;
    }
    if (aPollFd.revents & acHupMask) {
        return (int)TSRV_ERR_CONN_CLOSED;
    }
    if (aPollFd.revents & (0|POLLERR|POLLNVAL)) {
        return (int)TSRV_ERR_GENERAL;
    }
#ifdef _GNU_SOURCE
    acceptedFd = accept4(sockFd, &peerAddr, &peerAddrLen, 0|SOCK_NONBLOCK|SOCK_CLOEXEC);
#else      
    acceptedFd = accept(sockFd, &peerAddr, &peerAddrLen);
#endif
    acceptErrno = errno;
    if (acceptedFd > 0) {
        //connect(acceptedFd, &peerAddr, peerAddrLen);
        getnameinfo(&peerAddr, peerAddrLen, hostName, 256, svcName, 32, 0|NI_NUMERICHOST|NI_NUMERICSERV);
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

    int sendErrno = 0;
    ssize_t sendHeadIndex= 0;
    ssize_t sendRemain = 0;
    ssize_t sentLength = 0;
    int timeoutCount = 0;
    int pollResult = 0;
    ssize_t pollErrno = 0;
    struct pollfd stPoll = {};

    sendHeadIndex = 0;
    sendRemain = bufLength;
    try {
        stPoll.events = sPollFlag;
        stPoll.fd = cList.at(cID);
    } catch (std::exception& e) {
        printf("Send error: %s\n", e.what());
        return TSRV_ERR_GENERAL;
    }
    while ((sendRemain > 0) && !servTerminate) {
        pollResult = poll(&stPoll, 1, TSRV_TIMEOUT_MSEC);
        pollErrno = errno;
        if (pollResult == -1) {
            try {
                close(stPoll.fd);
                cList.erase(cID);
            } catch (std::exception& e) {
                printf("Client erased: %s\n", e.what());
            }
            return (ssize_t)(pollErrno * -1);
        }
        if (pollResult == 0) {
            timeoutCount++;
            if (timeoutCount > timeoutCountMax) {
                return TSRV_ERR_TIMEOUT;
            }
        }
        if (pollResult > 0) {
            printf("Sending to %d...\n", stPoll.fd);
            if (sendHeadIndex >= bufLength) {
                break;
            }
            if (stPoll.revents & (0|POLLOUT)) {
                sentLength = sendto(stPoll.fd, &(sBuffer[sendHeadIndex]), sendRemain, 0, nullptr, 0);
                sendErrno = errno;
                if (sentLength > 0) {
                    sendHeadIndex += sentLength;
                    sendRemain -= sentLength;
                } else {
                    switch(sendErrno){
                        case EAGAIN:
                            break;
                        default:
                            //close(stPoll.fd);
                            return (sendErrno * -1);
                    }
                }
            } else {
                if (stPoll.revents & (0|POLLHUP)) {
                    cList.erase(cID);
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
                }
            }
        }
    }
    return sendHeadIndex;
}

ssize_t TCPServer::recvFrom(int32_t cID, uint8_t* rBuffer, uint32_t bufLength) {
    ssize_t rfBytesLength = 0;
    ssize_t rfPollErrno = 0;
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
    rfPollErrno = (ssize_t)errno;
    if (rfPollRes == -1) {
        close(rfPoll.fd);
        return (ssize_t)(rfPollErrno * -1);
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
        rfBytesLength = recvfrom(rfPoll.fd, rBuffer, bufLength, 0, nullptr, 0);
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