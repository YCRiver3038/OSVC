#include "getopt.h"
#include "stdio.h"
#include <string>
#include <atomic>
#include "signal.h"
//#include "network.hpp"
#include "AudioManipulator.hpp"

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
        {"list-devices", no_argument, 0, 1000},
        {"addr", required_argument, 0, 2001},
        {"port", required_argument, 0, 2002},
        {"input-device", required_argument, 0, 2003},
        {"output-device", required_argument, 0, 2004},
        {"fsample", required_argument, 0, 2005},
        {"format", required_argument, 0, 2006},
        {"chunklength", required_argument, 0, 2007},
        {"rblength", required_argument, 0, 2008},
        {"duplex", no_argument, 0, 2009},
        {"server", no_argument, 0, 2010},
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
                printf("--list-devices              list all audio devices\n");
                printf("--server                    audio server mode\n");
                printf("--duplex                    audio duplex mode\n");
                printf("--chunklength [len: int]    set chunk length\n");
                printf("--rblength [len: int]       set ringbuffer length\n");
                printf("--input-device [idx: int]   set input device by index\n");
                printf("                            (shown by --list-devices)\n");
                printf("--output-device [idx: int]  set output device by index\n");
                printf("                            (shown by --list-devices)\n");
                printf("--fsample [fs: float]       set sampling frequency\n");
                printf("--format [fmt: string]      set data format\n");
                printf("Available format is below:\n");
                //printf("    8: Signed 8bit int\n");
                printf("    16: Signed 16bit int\n");
                //printf("    24: Signed 24bit int\n");
                printf("    32: Signed 32bit int\n");
                printf("    f32: 32bit float\n");
                return 0;
            case 1000: {
                AudioManipulator initOnly;
                initOnly.getPaVersion();
                initOnly.listDevices();
                return 0;
            }
            case 2001:
                conAddr.clear();
                conAddr.assign(optarg);
                break;
            case 2002:
                conPort.clear();
                conPort.assign(optarg);
                break;
            case 2003:
                try {
                    iDeviceIndex = std::stoi(std::string(optarg));
                } catch (std::invalid_argument) {
                    printf("Input: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2004:
                try {
                    oDeviceIndex = std::stoi(std::string(optarg));
                } catch (std::invalid_argument) {
                    printf("Output: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2005:
                try {
                    ioFs = std::stod(std::string(optarg));
                } catch (std::invalid_argument) {
                    printf("IO: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2006:
                ioFormat.assign(optarg);
                break;
            case 2007:
                try {
                    ioChunkLength = std::stoul(std::string(optarg));
                } catch (std::invalid_argument) {
                    printf("IO: Invalid length ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2008:
                try {
                    ioRBLength = std::stoul(std::string(optarg));
                } catch (std::invalid_argument) {
                    printf("IO: Invalid length ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2009:
                isDuplex = true;
                break;
            case 2010:
                isListener = false;
                break;
            default:
                break;
        }
    } while (getoptStatus != -1);

    AudioManipulator* aIO = nullptr;
    if (!isListener){
        printf("Input preparation...\n");
        aIO =  new AudioManipulator(iDeviceIndex, "i", ioFs, ioFormat, 2, ioRBLength, ioChunkLength);
        if (aIO->getInitStatus() != 0) {
            printf("Portaudio initialization error: %d\n", aIO->getInitStatus());
            delete aIO;
            return -1;
        }
    } else {
        printf("Output preparation...\n");
        aIO = new AudioManipulator(oDeviceIndex, "o", ioFs, ioFormat, 2, ioRBLength, ioChunkLength);
        if (aIO->getInitStatus() != 0) {
            printf("Portaudio initialization error: %d\n", aIO->getInitStatus());
            delete aIO;
            return -1;
        }
    }

    printf("Set addr: %s, port: %s\n", conAddr.c_str(), conPort.c_str());
    int conStatus = 0;
    //network con1(conAddr, conPort, AF_INET, SOCK_DGRAM);
    //if (isListener) {
    //    conStatus = con1.nw_bind_and_listen(true);
    //}
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
        delete aIO;
        return -1;
    }
    //sockaddr acceptedClient = {};
    //socklen_t acSockLen = 0;
    int acFileDesc = 0;
    printf("Ctrl+c to exit\n");
    AudioData rData[16384] = {0};
    AudioData tData[16384] = {0};
    ssize_t rLength = 0;
    aIO->start();
    while (!KeyboardInterrupt.load()) {
        //if (isListener) {
        //    rLength = con1.recv_data(&(rData->u8[0]), 16384);
        //    aIO->write(rData, rLength/(2*4));
        //} else {
        //    aIO->read(tData, ioChunkLength);
        //    con1.send_data(&(tData->u8[0]), ioChunkLength*2*4);
        //}
    }
    aIO->stop();
    delete aIO;
    //con1.nw_close();
    return 0;
}