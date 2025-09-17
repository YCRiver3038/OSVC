#include "getopt.h"
#include "stdio.h"
#include <string>
#include <atomic>
#include "signal.h"
#include <vector>
#include <iostream>

#include "network.hpp"
#include "remocon.h"

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

    uint32_t ioChunkLength = 4096;
    uint32_t ioRBLength = 16384;
    double ioFs = 48000.0;
    std::string ioFormat("f32");

    do {
        getoptStatus = getopt_long(argc, argv, "", long_options, &optionIndex);
        switch (getoptStatus) {
            case 'h':
                printf("usage: nwtest [args]\n");
                printf("args:\n");
                printf("--addr connect address\n");
                printf("--port connect port\n");
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
    network con1(conAddr, conPort);
    std::string conErrorText;
    conErrorText.clear();

    char destAddr[1024] = {};
    char destPort[16] = {};
    int getAddrStatus = 0;
    int conStatus = 0;
    conStatus = con1.nw_connect();
    getAddrStatus = con1.get_connection_addr(destAddr, 1024, destPort, 16);
    printf("Address: %s, port: %s\n", destAddr, destPort);
    if (conStatus != 0) {
        printf("Connect error: %d (%s)\n", conStatus, strerror(-1*conStatus));
        return -1;
    }

    int acFileDesc = 0;
    printf("'q' to exit\n");
    ssize_t rLength = 0;
    std::string command;
    std::vector<std::string> comlist;
    char msg[256] = {};

    float rbPitchScale = 0.0;
    float rbFormantScale = 0.0;
    float rbTimeRatio = 0.0;

    uint8_t tbuf[1024] = {};
    uint8_t rbuf[1024] = {};
    trdtype tdata;
    tdata.u32 = 0;
    while (!KeyboardInterrupt.load()){
        printf("OSVC > ");
        std::cin >> command;
        if (command == "") {
            printf("\nEOF\n");
            break;
        }
        //printf("input: %s", command.c_str());
        for (size_t ctrc=0; ctrc<command.length(); ctrc++) {
            command.at(ctrc) = std::tolower(command.at(ctrc));
        }
        if (command == "q") {
            KeyboardInterrupt.store(true);
        }
        if (command == "p") {
           printf("Pitch scale > ");
            std::cin >> command;
            try {
                rbPitchScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printf("Invalid value");
                continue;
            }
            tdata.f32 = rbPitchScale;
            tbuf[0] = SET_PITCH;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
        }
        if (command == "f") {
            printf("Formant scale > ");
            std::cin >> command;
            try {
                rbFormantScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printf("Invalid value");
                continue;
            }   
            tdata.f32 = rbFormantScale;
            tbuf[0] = SET_FORMANT;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
        }
        if (command == "t") {
            printf("Time ratio > ");
            std::cin >> command;
            try {
                rbTimeRatio = (std::stod(command.c_str()) > 0.0) ? std::stod(command.c_str()) : 1.0;
            } catch (const std::invalid_argument& e) {
                printf("Invalid value");
                continue;
            }
            tdata.f32 = rbTimeRatio;
            tbuf[0] = SET_TR;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
        }
        command.clear();
    }
    con1.nw_close();
    return 0;
}