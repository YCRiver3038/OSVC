#include "getopt.h"
#include "stdio.h"
#include <string>
#include <atomic>
#include "signal.h"
#include <vector>
#include <iostream>
#include <thread>

#include "math.h"

#include "network.hpp"
#include "remocon.h"

std::atomic<bool> KeyboardInterrupt;
void kbiHandler(int signo) {
    KeyboardInterrupt.store(true);
}

void rcvthr(network& rc) {
    uint8_t rbuf[1024] = {};
    ssize_t rcStatus = 0;
    std::string statstr;
    trdtype rdata;
    while (!KeyboardInterrupt.load()) {
        memset(rbuf, 0, 1024);
        rcStatus = rc.recv_data(rbuf, 1024);
        if (rcStatus < 0) {
            if (rcStatus != EM_CONNECTION_TIMEDOUT) {
                rc.errno_to_string(rcStatus, statstr);
                printf("\nrecv status: %s ( %zd )\n", statstr.c_str(), rcStatus);
                rc.nw_close();
                KeyboardInterrupt.store(true);
                break;
            }
            continue;
        }
        if (rcStatus == 0) {
            printf("Disconnected from server.\n");
            break;
        }
        //printf("Received:\n\x1b[0k");
        //for (ssize_t rdctr = 0; rdctr < rcStatus; rdctr++) {
        //    printf("%02X ", rbuf[rdctr]);
        //}
        //printf("\n");
        for (ssize_t rdctr = 0; rdctr < rcStatus;) {
            switch (rbuf[rdctr]) {
                case SRV_MSG:
                    rdctr++;
                    printf("%s", &(rbuf[rdctr]));
                    fflush(stdout);
                    while (rbuf[rdctr] != 0) {
                        rdctr++;
                        if (rdctr >= rcStatus) {
                            break;
                        }
                    }
                    break;
                case INFO_PITCH:     // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Pitch: %f\n", rdata.f32);
                    break;
                case INFO_FORMANT:   // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Formant: %f\n", rdata.f32);
                    break;
                case INFO_TR:        // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Time ratio: %f\n", rdata.f32);
                    break;
                case INFO_INPUT_LEVEL_DB:     // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Input Level: %f[dBFS]\n", rdata.f32);
                    break;
                case INFO_INPUT_LEVEL_NUM:    // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Input Level: %f\n", rdata.f32);
                    break;
                case INFO_INPUT_DEVICE:  // value: Text/json
                    rdctr++;
                    break;
                case INFO_OUTPUT_LEVEL_DB:    // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Output Level: %f[dBFS]\n", rdata.f32);
                    break;
                case INFO_OUTPUT_LEVEL_NUM:   // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Output Level: %f\n", rdata.f32);
                    break;
                case INFO_OUTPUT_VOLUME:   // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Output Volume: %f\n", rdata.f32);
                    break;
                case INFO_OUTPUT_DEVICE: // value: Text/json
                    rdctr++;
                    break;
                case INFO_LATENCY_MSEC:       // value: float32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Latency: %f[msec]\n", rdata.f32);
                    break;
                case INFO_LATENCY_SAMPLES:    // value: unsigned int32
                    memcpy(rdata.u8, &(rbuf[rdctr+1]), 4);
                    rdctr += 5;
                    printf("Info: Latency: %u[sample]\n", rdata.u32);
                    break;
                default:
                    rdctr++;
                    break;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    KeyboardInterrupt.store(false);

#if defined(_WIN32) || defined(_WIN64)
    signal(SIGINT, kbiHandler);
#else 
    struct sigaction sa = {};
    sa.sa_handler = kbiHandler;
    sigaction(SIGINT, &sa, nullptr);
#endif

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"addr", required_argument, 0, 2001},
        {"port", required_argument, 0, 2002},
        {0, 0, 0, 0}
    };
    int getoptStatus = 0;
    int optionIndex = 0;
    std::string conAddr("127.0.0.1");
    std::string conPort("63297");

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
    std::thread rcv(rcvthr, std::ref(con1));

    ssize_t rLength = 0;
    std::string command;
    std::vector<std::string> comlist;
    char msg[256] = {};

    float rbPitchScale = 0.0;
    float rbFormantScale = 0.0;
    float rbTimeRatio = 0.0;
    float oVol = 1.0;

    uint8_t tbuf[1024] = {};
    trdtype tdata;
    tdata.u32 = 0;

    while (!KeyboardInterrupt.load()){
        std::cin >> command;
        if (std::cin.eof()) {
            printf("\nEOF\n");
            break;
        }
        //printf("input: %s\n", command.c_str());
        for (size_t ctrc=0; ctrc<command.length(); ctrc++) {
            command.at(ctrc) = std::tolower(command.at(ctrc));
        }
        if (command == "q") {
            con1.nw_close();
            KeyboardInterrupt.store(true);
            continue;
        }
        if (command == "p") {
            printf("Pitch scale > ");
            std::cin >> command;
            try {
                rbPitchScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printf("Invalid value\n");
                continue;
            }
            tdata.f32 = rbPitchScale;
            tbuf[0] = SET_PITCH;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
            continue;
        }
        if (command == "f") {
            printf("Formant scale > ");
            std::cin >> command;
            try {
                rbFormantScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printf("Invalid value\n");
                continue;
            }   
            tdata.f32 = rbFormantScale;
            tbuf[0] = SET_FORMANT;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
            continue;
        }
        if (command == "t") {
            printf("Time ratio > ");
            std::cin >> command;
            try {
                rbTimeRatio = (std::stod(command.c_str()) > 0.0) ? std::stod(command.c_str()) : 1.0;
            } catch (const std::invalid_argument& e) {
                printf("Invalid value\n");
                continue;
            }
            tdata.f32 = rbTimeRatio;
            tbuf[0] = SET_TR;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
            continue;
        }
        if (command == "ov") {
            printf("Output Volume > ");
            std::cin >> command;
            try {
                oVol = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printf("Invalid value\n");
                continue;
            }
            tdata.f32 = oVol;
            tbuf[0] = SET_OUTPUT_VOLUME;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
            continue;
        }
        if (command == "ovd") {
            printf("Output Volume [dB] > ");
            std::cin >> command;
            try {
                oVol = std::stod(command.c_str());
                oVol = pow(10, oVol/20.0);
            } catch (const std::invalid_argument& e) {
                printf("Invalid value\n");
                continue;
            }
            tdata.f32 = oVol;
            tbuf[0] = SET_OUTPUT_VOLUME;
            memcpy(&(tbuf[1]), tdata.u8, 4);
            con1.send_data(tbuf, 5);
            continue;
        }
        if (command == "tpf") { // pitch follows time scale
            tbuf[0] = SET_TR_PITCH_FOLLOW;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "tpc") { // pitch won't follow time scale
            tbuf[0] = SET_TR_PITCH_CONSTANT;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qp") {
            tbuf[0] = QUERY_PITCH;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qf") {
            tbuf[0] = QUERY_FORMANT;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qt") {
            tbuf[0] = QUERY_TR;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qip") {
            tbuf[0] = QUERY_INPUT_LEVEL_DB;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qop") {
            tbuf[0] = QUERY_OUTPUT_LEVEL_DB;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qov") {
            tbuf[0] = QUERY_OUTPUT_VOLUME;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qlm") {
            tbuf[0] = QUERY_LATENCY_MSEC;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "qls") {
            tbuf[0] = QUERY_LATENCY_SAMPLES;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "thru") {
            tbuf[0] = MODE_THRU;
            con1.send_data(tbuf, 1);
            continue;
        }
        if (command == "proc") {
            tbuf[0] = MODE_PROCESSED;
            con1.send_data(tbuf, 1);
            continue;
        }
        printf("Invalid command: %s\n", command.c_str());
        printf("     > ");
        command.clear();
    }
    con1.nw_close();
    rcv.join();
    return 0;
}