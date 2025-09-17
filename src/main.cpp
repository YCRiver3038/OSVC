#include "main.h"
//#include "nlohmann/json.hpp"
#include "network.hpp"

#include "remocon.h"

std::atomic<bool> KeyboardInterrupt;
void kbiHandler(int signo) {
    KeyboardInterrupt.store(true);
}

template <typename FDTYPE, typename MDTYPE>
void printRatBar(FDTYPE fillLength, MDTYPE maxLength,
                 int charLength=10, bool withPercent=false,
                 char fillChar='#', char emptyChar='_',
                 bool colored=false, bool revcolor=false) {
    float fillRatio = 0.0;
    int fillCount = 0;
    if (fillLength > 0) {
        fillRatio = (float)fillLength / (float)maxLength;
    }
    fillCount = (int)(fillRatio * (float)charLength);
    if (fillCount > charLength) {
        fillCount = charLength;
    }
    bool halfMarked = false;
    bool fourFifthMarked = false;
    bool almostFullMarked = false;
    for (int ctr = 0; ctr < fillCount; ctr++) {
        if (colored) {
            if ((ctr < (charLength/2)) && !halfMarked) {
                if (revcolor) {
                    printf("\x1b[042m\x1b[097m");
                } else {
                    printf("\x1b[041m\x1b[097m");
                } 
                halfMarked = true;
            }
            if (((charLength/2) <= ctr) && (ctr < ((4*charLength)/5)) && !fourFifthMarked) {
                printf("\x1b[0m");
                printf("\x1b[043m\x1b[097m");
                fourFifthMarked = true;
            }
            if ( (((4*charLength)/5) <= ctr) && !almostFullMarked ) {
                printf("\x1b[0m");
                if (revcolor) {
                    printf("\x1b[041m\x1b[097m");
                } else {
                    printf("\x1b[042m\x1b[097m");
                }
                almostFullMarked = true;
            }
        }
        putchar(fillChar);
    }
    if (colored) {
        printf("\x1b[0m");
    }
    for (int ctr = 0; ctr < (charLength - fillCount); ctr++) {
        putchar(emptyChar);
    }
    if (withPercent) {
        printf("|%5.1f%%", fillRatio*100.0);
    }
}

void updateMax(float val, float* dest){
    if (val < 0.0) {
        val *= -1;
    }
    if (*dest < val) {
        *dest = val;
    }
}

void printToPlace(int row, int col, std::string& txt) {
    printf("\x1b[%d;%dH%s\x1b[0K", row, col, txt.c_str());
}

void printToPlace(int row, int col, const char* txt, size_t length) {
    std::string msg(txt, length);
    printf("\x1b[%d;%dH%s\x1b[0K", row, col, msg.c_str());
}

std::atomic<double> rbPitchScale = 1.0;
std::atomic<double> rbFormantScale = 1.0;
std::atomic<double> rbTimeRatio = 1.0;
std::atomic<float> ioLatency = 0.0;
std::atomic<uint32_t> ioLatencySamples = 0;
std::atomic<float> iPeak = 0;
std::atomic<float> oPeak = 0;
std::atomic<bool> isTHRU = false;

void rcom(std::string addr, std::string port) {
    network rcsrv(addr, port);
    fd_network* rc = nullptr;
    uint8_t rbuf[16384] = {};
    uint8_t sbuf[16384] = {};
    trdtype rdata;
    trdtype tdata;
    ssize_t rdstatus = 0;
    bool tr_pitch_follow = false;
    int acfd = 0;

    tdata.u32 = 0;
    rdata.u32 = 0;
    rdstatus = rcsrv.nw_bind_and_listen();

    printf("\nAddress: %s, port: %s\n", addr.c_str(), port.c_str());
    printf("bind status: %zd\n", rdstatus);
    if (rdstatus != 0) {
        return;
    }
    while (!KeyboardInterrupt.load()) {
        acfd = rcsrv.nw_accept();
        if (acfd > 0) {
            rc = new fd_network(acfd);
            printf("Client %d accepted\n", acfd);
            
            while (!KeyboardInterrupt.load()) {
                rdstatus = rc->recv_data(rbuf, 16384);
                if (rdstatus <= 0) {
                    if (rdstatus != EM_CONNECTION_TIMEDOUT) {
                        printf("Status: %zd\n", rdstatus);
                        break;
                    }
                } else {
                    printToPlace(8, 1, " ", 2);
                    printf("\nReceived:\x1b[0K\n");
                    for (ssize_t rctr=0; rctr < rdstatus; rctr++) {
                        printf("%02X ", rbuf[rctr]);
                    }
                    printf("\x1b[0K\n");
                }
                for (ssize_t rctr=0; rctr < rdstatus;) {
                    switch (rbuf[rctr]) {
                        case SET_PITCH: // value: float32
                            memcpy(rdata.u8, &(rbuf[rctr+1]), 4);
                            rbPitchScale.store((double)((rdata.f32 > 0.0) ? rdata.f32 : rbPitchScale.load()));
                            rctr += 5;
                            break;
                        case SET_FORMANT: // value: float32
                            memcpy(rdata.u8, &(rbuf[rctr+1]), 4);
                            rbFormantScale.store((double)((rdata.f32 > 0.0) ? rdata.f32 : rbFormantScale.load()));
                            rctr += 5;
                            break;
                        case SET_TR: // value: float32
                            memcpy(rdata.u8, &(rbuf[rctr+1]), 4);
                            rbTimeRatio.store((double)((rdata.f32 > 0.0) ? rdata.f32 : rbTimeRatio.load()));
                            if (tr_pitch_follow) {
                                rbPitchScale.store((rbTimeRatio.load() > 0.0) ? (1/rbTimeRatio.load()) : rbPitchScale.load());
                            }
                            rctr += 5;
                            break;
                        case SET_TR_PITCH_FOLLOW: // no following value
                            tr_pitch_follow = true;
                            rctr++;
                            break;
                        case SET_TR_PITCH_CONSTANT: // no following value
                            tr_pitch_follow = false;
                            rctr++;
                            break;
                        case SET_TR_CHANGE_DURATION: // value: float32
                            rctr += 5;
                            break;
                        case QUERY_PITCH:   // No following value
                            sbuf[0] = (uint8_t)INFO_PITCH;
                            tdata.f32 = (float)(rbPitchScale.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_FORMANT: // No following value
                            sbuf[0] = (uint8_t)INFO_FORMANT;
                            tdata.f32 = (float)(rbFormantScale.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_TR: // No following value
                            sbuf[0] = (uint8_t)INFO_TR;
                            tdata.f32 = (float)(rbTimeRatio.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_INPUT_LEVEL_DB: // No following value
                            sbuf[0] = (uint8_t)INFO_INPUT_LEVEL_DB;
                            tdata.f32 = 20.0*log10f(iPeak.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_INPUT_LEVEL_NUM: // No following value
                            sbuf[0] = (uint8_t)INFO_INPUT_LEVEL_NUM;
                            tdata.f32 = (float)(iPeak.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_OUTPUT_LEVEL_DB: // No following value
                            sbuf[0] = (uint8_t)INFO_OUTPUT_LEVEL_DB;
                            tdata.f32 = 20.0*log10f(oPeak.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_OUTPUT_LEVEL_NUM: // No following value
                            sbuf[0] = (uint8_t)INFO_OUTPUT_LEVEL_NUM;
                            tdata.f32 = (float)(oPeak.load());
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_LATENCY_MSEC: // No following value
                            sbuf[0] = (uint8_t)INFO_LATENCY_MSEC;
                            tdata.f32 = ioLatency.load();
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_LATENCY_SAMPLES: // No following value
                            sbuf[0] = (uint8_t)INFO_LATENCY_SAMPLES;
                            tdata.u32 = ioLatencySamples.load();
                            memcpy(&(sbuf[1]), tdata.u8, 4);
                            if (rc->send_data(sbuf, 5) <= 0){
                                printf("Send error.\n");
                            }
                            rctr++;
                            break;
                        case QUERY_INPUT_DEVICE: // No following value
                            sbuf[0] = (uint8_t)INFO_INPUT_DEVICE;
                            rctr++;
                            break;
                        case QUERY_OUTPUT_DEVICE: // No following value
                            sbuf[0] = (uint8_t)INFO_OUTPUT_DEVICE;
                            rctr++;
                            break;
                        case REC_START: // No following value
                            rctr++;
                            break;
                        case REC_STOP: // No following value
                            rctr++;
                            break;
                        case MODE_THRU: // No following value
                            isTHRU = true;
                            rctr++;
                            break;
                        case MODE_PROCESSED: //No following value
                            isTHRU = false;
                            rctr++;
                            break;
                        case STREAM_LENGTH: // value: unsigned int32
                            rctr += 5;
                            break;
                        default:
                            rctr++;
                            break;
                    }
                }
            }
            delete rc;
            printf("Client %d leaved\n", acfd);
        }
    }
}

void showHelp() {
    printf("usage: palb [args]\n");
    printf("args:\n");
    printf("--help                      show this message\n");
    printf("--list-devices              list all audio devices\n");
    printf("--chunklength [len: int]    set chunk length\n");
    printf("--rblength [len: int]       set ringbuffer length\n");
    printf("--input-device [idx: int]   set input device by index\n");
    printf("                            (shown by --list-devices)\n");
    printf("--output-device [idx: int]  set output device by index\n");
    printf("                            (shown by --list-devices)\n");
    printf("--fsample [fs: float]       set sampling frequency\n");
    printf("--init-pitch                set initial pitch scale\n");
    printf("--init-formant              set initial formant scale\n");
    printf("--mono                      monaural output\n");
    printf("--ll                        Low latency mode\n");
    printf("--thru                      Pass through mode (no processing)\n");
    printf("--tr-mix                    (Rubberband flag) TransientsMixed\n");
    printf("--tr-smooth                 (Rubberband flag) TransientsSmooth\n");
    printf("--smoothing                 (Rubberband flag) SmoothingOn\n");
    printf("--smoothing                 (Rubberband flag) ChannelsTogether\n");
    printf("\n--show-buffer-health             (Debug) Show buffer health in bar\n");
    printf("--barlength [len: int]           (Debug) Bar length\n");
    printf("                                         (intended for using with --show-buffer-length)\n");
    printf("--show-interval [interval: int]  (Debug) interval of health showing in microseconds\n");
    printf("                                         (intended for using with --show-buffer-length)\n");
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"list-devices", no_argument, 0, 1000},
        {"show-buffer-health", no_argument, 0, 1001},
        {"input-device", required_argument, 0, 2001},
        {"output-device", required_argument, 0, 2002},
        {"fsample", required_argument, 0, 2003},
        //{"format", required_argument, 0, 2004},
        {"chunklength", required_argument, 0, 2005},
        {"rblength", required_argument, 0, 2006},
        {"barlength", required_argument, 0, 2007},
        {"show-interval", required_argument, 0, 2008},
        {"init-pitch", required_argument, 0, 3001},
        {"init-formant", required_argument, 0, 3002},
        {"bind-addr", required_argument, 0, 4001},
        {"bind-port", required_argument, 0, 4002},
        {"stream", no_argument, 0, 5001},
        {"stream-bind-addr", required_argument, 0, 5002},
        {"stream-bind-port", required_argument, 0, 5003},
        {"mono", no_argument, 0, 9001},
        {"tr-mix", no_argument, 0, 9003},
        {"tr-smooth", no_argument, 0, 9004},
        {"smoothing", no_argument, 0, 9005},
        {"ll", no_argument, 0, 9006},
        {"ch-together", no_argument, 0, 9007},
        {"thru", no_argument, 0, 10001},
        {"sleep-time", required_argument, 0, 10002},
        {0, 0, 0, 0}
    };

#if defined(_WIN32) || defined(_WIN64)
    signal(SIGINT, kbiHandler);
#else 
    struct sigaction sa = {};
    sa.sa_handler = kbiHandler;
    sigaction(SIGINT, &sa, nullptr);
#endif

    bool listDevices = false;
    bool showBufferHealth = false;
    bool isMonoMode = false;
    int getoptStatus = 0;
    int optionIndex = 0;
    int iDeviceIndex = 0;
    int oDeviceIndex = 0;
    uint32_t ioChunkLength = 4096;
    uint32_t ioRBLength = 16384;
    double ioFs = 48000.0;
    std::string ioFormat("f32");
    int barMaxLength = 10;
    int showInterval = 50000000;

    struct timespec sleepTime = {0};
    sleepTime.tv_nsec = 500000; //1msec
    
    std::string rcomBindAddr("127.0.0.1");
    std::string rcomBindPort("63297");

    double cRbPitchScale = 1.0;
    double cRbFormantScale = 1.0;
    double cRbTimeRatio = 1.0;

    RubberBand::RubberBandStretcher::Options rbOptions;
    rbOptions = RubberBand::RubberBandStretcher::OptionProcessRealTime  |
                RubberBand::RubberBandStretcher::OptionFormantPreserved |
                RubberBand::RubberBandStretcher::OptionEngineFiner      |
                RubberBand::RubberBandStretcher::OptionTransientsCrisp  |
                RubberBand::RubberBandStretcher::OptionSmoothingOff     |
                RubberBand::RubberBandStretcher::OptionWindowStandard;

    do {
        getoptStatus = getopt_long(argc, argv, "", long_options, &optionIndex);
        switch (getoptStatus) {
            case '?':
                return -1;
            case 'h':
                showHelp();
                return 0;
            case 1000: {
                AudioManipulator initOnly;
                initOnly.getPaVersion();
                initOnly.listDevices();
                return 0;
            }
            case 1001: {
                showBufferHealth = true;
                break;
            }
            case 2001:
                try {
                    iDeviceIndex = std::stoi(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Input: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2002:
                try {
                    oDeviceIndex = std::stoi(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Output: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2003:
                try {
                    ioFs = std::stod(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Output: Invalid index ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2004:
                ioFormat.assign(optarg);
                // force set format to Float32 (since f32 is only the supported format)
                ioFormat.clear();
                ioFormat.assign("f32"); 
                break;
            case 2005:
                try {
                    ioChunkLength = std::stoul(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Output: Invalid length ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2006:
                try {
                    ioRBLength = std::stoul(std::string(optarg)) * 2;
                } catch (const std::invalid_argument& e) {
                    printf("Output: Invalid length ( %s )\n", optarg);
                    return -1;
                }
                break;
            case 2007:
                try {
                    barMaxLength = std::stoi(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Invalid length: %s\n", optarg);
                    return -1;
                }
                break;
            case 2008:
                try {
                    showInterval = std::stoi(std::string(optarg)) * 1000;
                } catch (const std::invalid_argument& e) {
                    printf("Invalid interval: %s\n", optarg);
                    return -1;
                }
                break;
            case 3001:
                try {
                    rbPitchScale.store(std::stod(std::string(optarg)));
                } catch (const std::invalid_argument& e) {
                    printf("Invalid value: %s\n", optarg);
                    return -1;
                }
                break;
            case 3002:
                try {
                    rbFormantScale.store(std::stod(std::string(optarg)));
                } catch (const std::invalid_argument& e) {
                    printf("Invalid value: %s\n", optarg);
                    return -1;
                }
                break;
            case 4001:
                rcomBindAddr.clear();
                rcomBindAddr.assign(optarg);
                break;
            case 4002:
                rcomBindPort.clear();
                rcomBindPort.assign(optarg);
                break;
            case 9001:
                isMonoMode = true;
                break;
            case 9003: //tr-mix
                rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsMixed;
                break;
            case 9004: //tr-smooth
                rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsSmooth;
                break;
            case 9005: //dsp-smoothing
                rbOptions |= RubberBand::RubberBandStretcher::OptionSmoothingOn;
                break;
            case 9006: //ll
                rbOptions |= RubberBand::RubberBandStretcher::OptionWindowShort;
                break;
            case 9007: //ch-together
                rbOptions |= RubberBand::RubberBandStretcher::OptionChannelsTogether;
                break;
            case 10001: //thru
                isTHRU.store(true);
                break;
            case 10002: //sleep-time
                try {
                    sleepTime.tv_nsec = std::stol(std::string(optarg)) * 1000;
                } catch (const std::invalid_argument& e) {
                    printf("Invalid value: %s\n", optarg);
                    return -1;
                }
                break;
            default:
                break;
        }
    } while (getoptStatus != -1);

    bool monoInput = false;

    printf("Output preparation...\n");
    AudioManipulator aOut(oDeviceIndex, "o",
                          ioFs, ioFormat, 2, ioRBLength, ioChunkLength);
    if (aOut.getInitStatus() != 0) {
        printf("Portaudio initialization error: %d\n", aOut.getInitStatus());
        return -1;
    }

    printf("Input preparation...\n");
    AudioManipulator aIn(iDeviceIndex, "i",
                         ioFs, ioFormat, 2, ioRBLength, ioChunkLength);
    if (aIn.getInitStatus() != 0) {
        printf("Portaudio initialization error: %d\n", aIn.getInitStatus());
        return -1;
    }

    if (aIn.getChannelCount() == 1) {
        monoInput = true;
    }

    unsigned long nSa = ioChunkLength*aIn.getChannelCount()*2;
    AudioData* tdarr = new AudioData[nSa];
    for (uint32_t ctr1 = 0; ctr1 < nSa; ctr1++) {
        tdarr[ctr1].f32 = 0.0;
    }

    //aIn.start();
    uint32_t aInEmptyCount  = 0;
    uint32_t aOutEmptyCount  = 0;
    uint32_t aInFullCount = 0;
    uint32_t aOutFullCount = 0;
    if (showBufferHealth) {
        printf("\nBuffer health\n");
    }
    uint32_t aInRbStoredLength = 0;
    uint32_t aOutRbStoredLength = 0;
    uint32_t aInRbLength = aIn.getRbChunkLength();
    uint32_t aOutRbLength = aOut.getRbChunkLength();
    uint32_t aOutStartThreshold = aOutRbLength / 2;
    uint32_t aInStartThreshold = aInRbLength / 2;
    bool isInit = false;
    int showCount = 0;
    bool aInRestarted = true;
    float iPeakM = 0.0;
    float oPeakM = 0.0;
    uint32_t ioChannel = aOut.getChannelCount();
    float** deinterleaved = new float*[ioChannel];
    float** rbResult = new float*[ioChannel];
    for (uint32_t ctr1 = 0; ctr1 < ioChannel; ctr1++) {
        deinterleaved[ctr1] = new float[ioChunkLength];
        rbResult[ctr1] = new float[ioChunkLength];
    }

    RubberBand::RubberBandStretcher* rbst1 = nullptr;
    if (!isTHRU.load()) {
        rbst1 = new RubberBand::RubberBandStretcher((size_t)ioFs, ioChannel, rbOptions);
    }

    printf("\x1b[2J\x1b[1;1H\x1b[0K== OSVC ==\n");
    std::thread rcomt(rcom, rcomBindAddr, rcomBindPort);

    aIn.start();
    aOut.start();
    aOut.pause();
    size_t rbLatancy = 0;
    char msg[256] = {};
    if (!isTHRU.load()) {
        rbst1->setMaxProcessSize(ioChunkLength);
    }
    while (!KeyboardInterrupt.load()) {
        nanosleep(&sleepTime, nullptr);
        showCount += sleepTime.tv_nsec;
        
        aInRbStoredLength = aIn.getRbStoredChunkLength();
        aOutRbStoredLength = aOut.getRbStoredChunkLength();
        if (!isInit) {
            if (aOutRbStoredLength >= aOutStartThreshold) {
                aOut.resume();
                isInit = true;
            }
        }
        if (aOutRbStoredLength == 0) {
            aOutEmptyCount++;
        }
        if (aInRbStoredLength >= ioChunkLength) {
            if (aInRbStoredLength >= aInRbLength) {
                aInFullCount++;
            }
            aIn.read(tdarr, ioChunkLength);
            if (aInRbStoredLength == 0) {
                aInEmptyCount++;
            }

            if (showBufferHealth) {
                for (uint32_t ctr = 0; ctr < (ioChunkLength*aOut.getChannelCount()); ctr++) {
                    updateMax(tdarr[ctr].f32, &iPeakM);
                }
                iPeak.store(iPeakM);
            }
            if (!monoInput) {
                AudioManipulator::deinterleave(tdarr, (AudioData**)deinterleaved, ioChunkLength);
            }  else {
                for (uint32_t idx = 0; idx < ioChunkLength; idx++) {
                    deinterleaved[0][idx] = tdarr[idx].f32;
                    deinterleaved[1][idx] = tdarr[idx].f32;
                }
            }
            if (!isTHRU) {
                rbst1->setFormantScale(rbFormantScale.load());
                rbst1->setPitchScale(rbPitchScale.load());
                rbst1->setTimeRatio(rbTimeRatio.load());
                rbst1->process(deinterleaved, ioChunkLength, false);
                rbLatancy = rbst1->getStartDelay();
                ioLatencySamples.store(rbLatancy+ioRBLength);
                ioLatency.store(((float)ioLatencySamples/ioFs)*1000.0);
                if (rbst1->available() > ioChunkLength) {
                    if (cRbTimeRatio != rbTimeRatio.load()) {
                        rbst1->reset();
                        cRbTimeRatio = rbTimeRatio.load();
                        snprintf(msg, 256, "\nTime ratio changed to %5.3lf\n", rbTimeRatio.load());
                        printToPlace(5, 1, msg, 256);
                    }
                    if (cRbFormantScale != rbFormantScale.load()) {
                        cRbFormantScale = rbFormantScale.load();
                        snprintf(msg, 256, "\nFormant scale changed to %5.3lf\n", rbFormantScale.load());
                        printToPlace(5, 1, msg, 256);
                    }
                    if (cRbPitchScale != rbPitchScale.load()) {
                        cRbPitchScale = rbPitchScale.load();
                        snprintf(msg, 256, "\nPitch scale changed to %5.3lf\n", rbPitchScale.load());
                        printToPlace(5, 1, msg, 256);
                    }
                    rbst1->retrieve(rbResult, ioChunkLength);
                    if (isMonoMode) {
                        for (uint32_t idx=0; idx<ioChunkLength; idx++) {
                            rbResult[0][idx] += rbResult[1][idx];
                            rbResult[0][idx] /= 2.0;
                            rbResult[1][idx] = rbResult[0][idx];
                        }
                    }
                    AudioManipulator::interleave((AudioData**)rbResult, tdarr, ioChunkLength);

                    aOut.write(tdarr, ioChunkLength);
                    if (aOutRbStoredLength >= aOutRbLength) {
                        aOutFullCount++;
                    } 

                    for (uint32_t ctr = 0; ctr < (ioChunkLength*aOut.getChannelCount()); ctr++) {
                        updateMax(tdarr[ctr].f32, &oPeakM);
                    }
                    oPeak.store(oPeakM);
                }
            } else {
                AudioManipulator::interleave((AudioData**)deinterleaved, tdarr, ioChunkLength);
                aOut.write(tdarr, ioChunkLength);
                if (showBufferHealth) {
                    for (uint32_t ctr = 0; ctr < (ioChunkLength*aOut.getChannelCount()); ctr++) {
                        updateMax(tdarr[ctr].f32, &oPeakM);
                    }
                    oPeak.store(oPeakM);
                }
            }
        }
        if ((showCount > showInterval) && showBufferHealth) {
            snprintf(msg, 256, " PARAM| P: %5.3lf | F: %5.3lf | T: %5.3lf| L: %6u (%5.1fmsec) | A: %6d",
            rbPitchScale.load(), rbFormantScale.load(), rbTimeRatio.load(), ioLatencySamples.load(), ioLatency.load(), rbst1->available());
            printToPlace(6, 1, msg, 256);
            printf("\x1b[7;1H INPUT|");
            printRatBar(iPeak.load(), 1.0, barMaxLength, false, ' ', ' ', true, true);
            printf("|%7.2fdB\x1b[0K", 20*log10f(iPeak.load()));
            printf("\nOUTPUT|");
            printRatBar(oPeak.load(), 1.0, barMaxLength, false, ' ', ' ', true, true);
            printf("|%7.2fdB\x1b[0K\n   IRB|", 20*log10f(oPeak.load()));
            printRatBar(aInRbStoredLength, aInRbLength, barMaxLength, true, '*', ' ', true, true);
            printf("| E(%d), O(%d) | %05lu\x1b[0K\n   ORB|", aInEmptyCount, aInFullCount, aIn.getRxCbFrameCount());
            printRatBar(aOutRbStoredLength, aOutRbLength, barMaxLength, true, '*', ' ', true);
            printf("| E(%d), O(%d) | %05lu\x1b[0K", aOutEmptyCount, aOutFullCount, aOut.getTxCbFrameCount());
            fflush(stdout);
            showCount = 0;
            iPeakM = 0.0;
            oPeakM = 0.0;
            iPeak.store(0.0);
            oPeak.store(0.0);
        }
    }
    if (KeyboardInterrupt.load()){
        printf("\n\n\nKeyboardInterrupt.\n");
    }
    aIn.stop();
    aOut.stop();
    printf("\x1b[2J\x1b[1;1H\x1b[0K\n");
    aIn.terminate();
    aOut.terminate();
    delete[] tdarr;
    for (uint32_t ctr1=0; ctr1<ioChannel; ctr1++) {
        delete[] deinterleaved[ctr1];
        delete[] rbResult[ctr1];
    }
    delete[] deinterleaved;
    delete[] rbResult;
    if (rbst1) { 
        delete rbst1;
    }
    rcomt.join();
    return 0;
}
