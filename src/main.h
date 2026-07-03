#include <iostream>
#include <string>
#include <thread>
#include <cmath>
//#include <atomic>
#include <vector>
#include <map>
#include <stdexcept>

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "math.h"

#include "getopt.h"
#include "time.h"
#include "signal.h"

#include "rubberband/RubberBandStretcher.h"
//#include "nlohmann/json.hpp"

//#include "network.hpp"
#include "TCPServer.hpp"
#include "UDPServer.hpp"
#include "UDPClient.hpp"
#include "remocon.h"
#include "AudioManipulator.hpp"

#include "SelfAtomic.h"

#if defined(_WIN32) || defined(_WIN64)
#else
#include "memMappedFile.h"
#endif

// Global Variables
SelfAtomic<double> rbPitchScale;
SelfAtomic<double> rbFormantScale;
SelfAtomic<double> rbTimeRatio;
SelfAtomic<float> ioLatency;
SelfAtomic<uint32_t> ioLatencySamples;
SelfAtomic<float> iPeak;
SelfAtomic<float> oPeak;
SelfAtomic<float> oVolume;
SelfAtomic<uint32_t> aInRbStoredLength;
SelfAtomic<uint32_t> aOutRbStoredLength;
SelfAtomicBool isTHRU;

#if defined(_WIN32) || defined(_WIN64)
#include "synchapi.h"
typedef DWORD suseconds_t;
#endif
