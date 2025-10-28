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
std::atomic<double> rbPitchScale = 1.0;
std::atomic<double> rbFormantScale = 1.0;
std::atomic<double> rbTimeRatio = 1.0;
std::atomic<float> ioLatency = 0.0;
std::atomic<uint32_t> ioLatencySamples = 0;
std::atomic<float> iPeak = 0;
std::atomic<float> oPeak = 0;
std::atomic<bool> isTHRU = false;
std::atomic<float> oVolume = 1.0;
std::atomic<uint32_t> aInRbStoredLength = 0;
std::atomic<uint32_t> aOutRbStoredLength = 0;
