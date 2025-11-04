#include <iostream>
#include <string>
#include <thread>
#include <cmath>
#include <atomic>
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

#include "AudioManipulator.hpp"

// Global Variables
std::atomic<double> rbPitchScale;
std::atomic<double> rbFormantScale;
std::atomic<double> rbTimeRatio;
std::atomic<float> ioLatency;
std::atomic<uint32_t> ioLatencySamples;
std::atomic<float> iPeak;
std::atomic<float> oPeak;
std::atomic<bool> isTHRU;
std::atomic<float> oVolume;
std::atomic<uint32_t> aInRbStoredLength;
std::atomic<uint32_t> aOutRbStoredLength;
