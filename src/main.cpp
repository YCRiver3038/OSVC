#include "main.h"
#include "network.hpp"
#include "nlohmann/json.hpp"

std::atomic<bool> KeyboardInterrupt;
void kbiHandler(int signo) {
    KeyboardInterrupt.store(true);
}

enum {
    SET_PITCH,
    SET_FORMANT,
    SET_TR,
    SET_TR_PITCH_FOLLOW,
    SET_TR_PITCH_CONSTANT,
    SET_TR_CHANGE_DURATION,
    QUERY_PITCH,
    QUERY_FORMANT,
    QUERY_TR,
    QUERY_INPUT_LEVEL_DB,
    QUERY_INPUT_LEVEL_NUM,
    QUERY_OUTPUT_LEVEL_DB,
    QUERY_OUTPUT_LEVEL_NUM,
    QUERY_LATENCY_MSEC,
    QUERY_LATENCY_SAMPLES,
    QUERY_INPUT_DEVICE_INFO,
    QUERY_OUTPUT_DEVICE_INFO,
    INFO_PITCH,
    INFO_FORMANT,
    INFO_TR,
    REC_START,
    REC_STO,
    MODE_THRU,
    MODE_PROCESSED
} sParams;

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

double rbPitchScale = 1.0;
double rbFormantScale = 1.0;
double rbTimeRatio = 1.0;
void comthr() {
    std::string command;
    std::vector<std::string> comlist;
    char msg[256] = {};
    while (!KeyboardInterrupt.load()){
        printToPlace(1, 1, "OSVC >", 6);
        std::cin >> command;
        for (auto com : comlist) {
            printf("%s | ", com.c_str());
        }
        snprintf(msg, 256, "input: %s", command.c_str());
        printToPlace(2, 1, msg, 256);
        for (size_t ctrc=0; ctrc<command.length(); ctrc++) {
            command.at(ctrc) = std::tolower(command.at(ctrc));
        }
        if (command == "q") {
            KeyboardInterrupt.store(true);
        }
        if (command == "p") {
            printToPlace(3, 1, "Pitch scale > ", 15);
            std::cin >> command;
            try {
                rbPitchScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printToPlace(4, 1, "Invalid value", 14);
            }   
        }
        if (command == "f") {
            printToPlace(3, 1, "Formant scale > ", 17);
            std::cin >> command;
            try {
                rbFormantScale = std::stod(command.c_str());
            } catch (const std::invalid_argument& e) {
                printToPlace(4, 1, "Invalid value", 14);
            }   
        }
        if (command == "t") {
            printToPlace(3, 1, "Time ratio > ", 14);
            std::cin >> command;
            try {
                rbTimeRatio = (std::stod(command.c_str()) > 0.0) ? std::stod(command.c_str()) : 1.0;
            } catch (const std::invalid_argument& e) {
                printToPlace(4, 1, "Invalid value", 14);
            }
        }
        command.clear();
    }
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"list-devices", no_argument, 0, 1000},
        {"show-buffer-health", no_argument, 0, 1001},
        {"input-device", required_argument, 0, 2001},
        {"output-device", required_argument, 0, 2002},
        {"fsample", required_argument, 0, 2003},
        {"format", required_argument, 0, 2004},
        {"chunklength", required_argument, 0, 2005},
        {"rblength", required_argument, 0, 2006},
        {"barlength", required_argument, 0, 2007},
        {"show-interval", required_argument, 0, 2008},
        {"init-pitch", required_argument, 0, 3001},
        {"init-formant", required_argument, 0, 3002},
        {"mono", no_argument, 0, 9001},
        {"tr-mix", no_argument, 0, 9003},
        {"tr-smooth", no_argument, 0, 9004},
        {"smoothing", no_argument, 0, 9005},
        {"ll", no_argument, 0, 9006},
        {"thru", no_argument, 0, 10001},
        {0, 0, 0, 0}
    };

#if defined(__linux__) || defined(__APPLE__)
    struct sigaction sa = {};
    sa.sa_handler = kbiHandler;
    sigaction(SIGINT, &sa, nullptr);
#else
    signal(SIGINT, kbiHandler);
#endif

    bool listDevices = false;
    bool showBufferHealth = false;
    bool isMonoMode = false;
    bool isTHRU = false;
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
                printf("\n--show-buffer-health             (Debug) Show buffer health in bar\n");
                printf("--barlength [len: int]           (Debug) Bar length\n");
                printf("                                         (intended for using with --show-buffer-length)\n");
                printf("--show-interval [interval: int]  (Debug) interval of health showing in microseconds\n");
                printf("                                         (intended for using with --show-buffer-length)\n");
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
                    rbPitchScale = std::stod(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Invalid value: %s\n", optarg);
                    return -1;
                }
                break;
            case 3002:
                try {
                    rbFormantScale = std::stod(std::string(optarg));
                } catch (const std::invalid_argument& e) {
                    printf("Invalid value: %s\n", optarg);
                    return -1;
                }
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
            case 10001: //thru
                isTHRU = true;
                break;
            default:
                break;
        }
    } while (getoptStatus != -1);

    bool monoInput = false;

    printf("Output preparation...\n");
    AudioManipulator aOut(oDeviceIndex, "o",
                          ioFs, ioFormat, 2, ioRBLength, ioChunkLength);
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
    if (aOut.getInitStatus() != 0) {
        printf("Portaudio initialization error: %d\n", aOut.getInitStatus());
        return -1;
    }

    struct timespec sleepTime = {0};
    sleepTime.tv_nsec = 500000; //1msec

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
    AudioData iPeak;
    AudioData oPeak;
    iPeak.f32 = 0.0;
    oPeak.f32 = 0.0;
    uint32_t ioChannel = aOut.getChannelCount();
    float** deinterleaved = new float*[ioChannel];
    float** rbResult = new float*[ioChannel];
    for (uint32_t ctr1 = 0; ctr1 < ioChannel; ctr1++) {
        deinterleaved[ctr1] = new float[ioChunkLength];
        rbResult[ctr1] = new float[ioChunkLength];
    }

    RubberBand::RubberBandStretcher* rbst1 = nullptr;
    if (!isTHRU) {
        rbst1 = new RubberBand::RubberBandStretcher((size_t)ioFs, ioChannel, rbOptions);
    }

    printf("\x1b[2J\n");
    std::thread com(comthr);
    aIn.start();
    aOut.start();
    aOut.pause();
    size_t rbLatancy = 0;
    char msg[256] = {};
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

            for (uint32_t ctr = 0; ctr < (ioChunkLength*aOut.getChannelCount()); ctr++) {
                if (ioFormat=="8") {
                    updateMax((float)tdarr[ctr].s8[0] / 128.0, &(iPeak.f32));
                }
                if (ioFormat=="16") {
                    updateMax((float)tdarr[ctr].s16[0] / 32768.0, &(iPeak.f32));
                }
                if (ioFormat=="32") {
                    updateMax((float)tdarr[ctr].s32 / 2147483647.0, &(iPeak.f32));
                }
                if (ioFormat=="f32") {
                    updateMax(tdarr[ctr].f32, &(iPeak.f32));
                }
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
                rbst1->setFormantScale(rbFormantScale);
                rbst1->setPitchScale(rbPitchScale);
                rbst1->setTimeRatio(rbTimeRatio);
                rbst1->process(deinterleaved, ioChunkLength, false);
                rbLatancy = rbst1->getStartDelay();
                snprintf(msg, 256, " PARAM| P: %5.3lf | F: %5.3lf | T: %5.3lf| L: %6ld (%5.1fmsec) | A: %6d",
                        rbPitchScale, rbFormantScale, rbTimeRatio, rbLatancy+ioRBLength, ((float)(rbLatancy+ioRBLength)/ioFs)*1000.0, rbst1->available());
                printToPlace(6, 1, msg, 256);
                if (rbst1->available() > ioChunkLength) {
                    if (cRbTimeRatio != rbTimeRatio) {
                        rbst1->reset();
                        cRbTimeRatio = rbTimeRatio;
                        snprintf(msg, 256, "Time ratio changed to %5.3lf", rbTimeRatio);
                        printToPlace(5, 1, msg, 256);
                    }
                    if (cRbFormantScale != rbFormantScale) {
                        cRbFormantScale = rbFormantScale;
                        snprintf(msg, 256, "Formant scale changed to %5.3lf", rbFormantScale);
                        printToPlace(5, 1, msg, 256);
                    }
                    if (cRbPitchScale != rbPitchScale) {
                        cRbPitchScale = rbPitchScale;
                        snprintf(msg, 256, "Pitch scale changed to %5.3lf", rbPitchScale);
                        printToPlace(5, 1, msg, 256);
                    }
                    rbst1->retrieve(rbResult, ioChunkLength);
                    rbst1->setMaxProcessSize(ioChunkLength);
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
                        if (ioFormat=="8") {
                            updateMax((float)tdarr[ctr].s8[0] / 128.0, &(oPeak.f32));
                        }
                        if (ioFormat=="16") {
                            updateMax((float)tdarr[ctr].s16[0] / 32768.0, &(oPeak.f32));
                        }
                        if (ioFormat=="32") {
                            updateMax((float)tdarr[ctr].s32 / 2147483647.0, &(oPeak.f32));
                        }
                        if (ioFormat=="f32") {
                            updateMax(tdarr[ctr].f32, &(oPeak.f32));
                        }
                    }
                }
            } else {
                AudioManipulator::interleave((AudioData**)deinterleaved, tdarr, ioChunkLength);
                aOut.write(tdarr, ioChunkLength);
                for (uint32_t ctr = 0; ctr < (ioChunkLength*aOut.getChannelCount()); ctr++) {
                    if (ioFormat=="8") {
                        updateMax((float)tdarr[ctr].s8[0] / 128.0, &(oPeak.f32));
                    }
                    if (ioFormat=="16") {
                        updateMax((float)tdarr[ctr].s16[0] / 32768.0, &(oPeak.f32));
                    }
                    if (ioFormat=="32") {
                        updateMax((float)tdarr[ctr].s32 / 2147483647.0, &(oPeak.f32));
                    }
                    if (ioFormat=="f32") {
                        updateMax(tdarr[ctr].f32, &(oPeak.f32));
                    }
                }
            }
        }
        if ((showCount > showInterval) && showBufferHealth) {
            printf("\x1b[7;1H INPUT|");
            printRatBar(iPeak.f32, 1.0, barMaxLength, false, ' ', ' ', true, true);
            printf("|%7.2fdB\x1b[0K", 20*log10f(iPeak.f32));
            printf("\nOUTPUT|");
            printRatBar(oPeak.f32, 1.0, barMaxLength, false, ' ', ' ', true, true);
            printf("|%7.2fdB\x1b[0K\n   IRB|", 20*log10f(oPeak.f32));
            printRatBar(aInRbStoredLength, aInRbLength, barMaxLength, true, '*', ' ', true, true);
            printf("| E(%d), O(%d) | %05lu\x1b[0K\n   ORB|", aInEmptyCount, aInFullCount, aIn.getRxCbFrameCount());
            printRatBar(aOutRbStoredLength, aOutRbLength, barMaxLength, true, '*', ' ', true);
            printf("| E(%d), O(%d) | %05lu\x1b[0K", aOutEmptyCount, aOutFullCount, aOut.getTxCbFrameCount());
            fflush(stdout);
            showCount = 0;
            iPeak.f32 = 0.0;
            oPeak.f32 = 0.0;
        }
    }
    if (KeyboardInterrupt.load()){
        printf("\n\n\nKeyboardInterrupt.\n");
    }
    aIn.stop();
    aOut.stop();

    delete[] tdarr;
    for (uint32_t ctr1=0; ctr1<ioChannel; ctr1++) {
        delete[] deinterleaved[ctr1];
        delete[] rbResult[ctr1];
    }
    delete[] deinterleaved;
    delete[] rbResult;
    delete rbst1;
    com.join();
    printf("\x1b[2J\n");
    return 0;
}
