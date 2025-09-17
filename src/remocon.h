#ifndef REMOCON_H_INCLUDED
#define REMOCON_H_INCLUDED

#include "stdint.h"

/*
Protocol: UDP
Data Format:
[cParams][VALUE][cParams][VALUE]...

VALUE: variable length
see comment section of sParams definitions 

Sending or receiving illegal format may lead to catastrophic behavior.
*/

enum {
    SET_PITCH,   // value: float32
    SET_FORMANT, // value: float32
    SET_TR,      // value: float32
    SET_TR_PITCH_FOLLOW,    // No following value
    SET_TR_PITCH_CONSTANT,  // No following value
    SET_TR_CHANGE_DURATION, // value: float32
    QUERY_PITCH,   // No following value
    QUERY_FORMANT, // No following value
    QUERY_TR,      // No following value
    QUERY_INPUT_LEVEL_DB,     // No following value
    QUERY_INPUT_LEVEL_NUM,    // No following value
    QUERY_OUTPUT_LEVEL_DB,    // No following value
    QUERY_OUTPUT_LEVEL_NUM,   // No following value
    QUERY_LATENCY_MSEC,       // No following value
    QUERY_LATENCY_SAMPLES,    // No following value
    QUERY_INPUT_DEVICE,  // No following value
    QUERY_OUTPUT_DEVICE, // No following value
    INFO_PITCH,     // value: float32
    INFO_FORMANT,   // value: float32
    INFO_TR,        // value: float32
    INFO_INPUT_LEVEL_DB,     // value: float32
    INFO_INPUT_LEVEL_NUM,    // value: float32
    INFO_OUTPUT_LEVEL_DB,    // value: float32
    INFO_OUTPUT_LEVEL_NUM,   // value: float32
    INFO_LATENCY_MSEC,       // value: float32
    INFO_LATENCY_SAMPLES,    // value: unsigned int32
    INFO_INPUT_DEVICE,  // value: Text/json
    INFO_OUTPUT_DEVICE, // value: Text/json
    REC_START,      // No following value
    REC_STOP,        // No following value
    MODE_THRU,      // No following value
    MODE_PROCESSED, //No following value
    STREAM_DATA,    //value: float32 array
    STREAM_LENGTH   // value: unsigned int32
} cParams; // rcParams itself is transmitted in format uint8

typedef union {
    uint8_t u8[4];
    uint8_t u16[2];
    uint32_t u32;
    float f32;
} trdtype;

#endif