#include "getopt.h"
#include "stdio.h"
#include <string>
#include <atomic>
#include "signal.h"
#include "network.hpp"

std::atomic<bool> KeyboardInterrupt;
void kbiHandler(int signo) {
    KeyboardInterrupt.store(true);
}

int main(int argc, char* argv[]) {
    KeyboardInterrupt.store(false);
    struct sigaction sa = {0};
    sa.sa_handler = kbiHandler;
    sigaction(SIGINT, &sa, nullptr);

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"addr", required_argument, 0, 2001},
        {"port", required_argument, 0, 2002},
        {0, 0, 0, 0}
    };
    int getoptStatus = 0;
    int optionIndex = 0;
    std::string conAddr("::1");
    std::string conPort("61234");
    bool listDevices = false;
    int oDeviceIndex = 0;
    int iDeviceIndex = 0;
    uint32_t ioChunkLength = 4096;
    uint32_t ioRBLength = 16384;
    double ioFs = 48000.0;
    std::string ioFormat("f32");
    bool isDuplex = false;
    bool isListener = true;
    do {
        getoptStatus = getopt_long(argc, argv, "", long_options, &optionIndex);
        switch (getoptStatus) {
            case 'h':
                printf("usage: nwtest [args]\n");
                printf("args:\n");
                printf("--addr                      connect address\n");
                printf("--port                      connect port\n");
                return 0;
            case 2001:
                conAddr.clear();
                conAddr.assign(optarg);
                break;
            case 2002:
                conPort.clear();
                conPort.assign(optarg);
                break;
            default:
                break;
        }
    } while (getoptStatus != -1);

    printf("Set addr: %s, port: %s\n", conAddr.c_str(), conPort.c_str());
    int conStatus = 0;
    network con1(conAddr, conPort);
    std::string conErrorText;
    conErrorText.clear();
    //con1.errno_to_string(conStatus, conErrorText);
    char destAddr[1024] = {};
    char destPort[16] = {};
    int getAddrStatus = 0;
    //getAddrStatus = con1.get_connection_addr(destAddr, 1024, destPort, 16);
    //printf("Address: %s, port: %s\n", destAddr, destPort);
    //if (!isListener) {
    //    printf("Connection status: %d (%s)\n", conStatus, conErrorText.c_str());
    //} else {
    //    printf("Bind status: %d (%s)\n", conStatus, conErrorText.c_str());
    //}
    if (conStatus != 0) {
        return -1;
    }
    //sockaddr acceptedClient = {};
    //socklen_t acSockLen = 0;
    int acFileDesc = 0;
    printf("Ctrl+c to exit\n");
    ssize_t rLength = 0;
    while (!KeyboardInterrupt.load()) {
        //if (isListener) {
        //    rLength = con1.recv_data(&(rData->u8[0]), 16384);
        //    aIO->write(rData, rLength/(2*4));
        //} else {
        //    aIO->read(tData, ioChunkLength);
        //    con1.send_data(&(tData->u8[0]), ioChunkLength*2*4);
        //}
    }
    con1.nw_close();
    return 0;
}