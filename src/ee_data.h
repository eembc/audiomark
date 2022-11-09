#ifndef __EE_DATA_H
#define __EE_DATA_H

#define DATA_SIZE (211 * 128)

const int16_t downlink_audio[DATA_SIZE] = {
#include "ee_data/noise.txt"
};
const int16_t left_microphone_capture[DATA_SIZE] = {
#include "ee_data/left0.txt"
};
const int16_t right_microphone_capture[DATA_SIZE] = {
#include "ee_data/right0.txt"
};

static int16_t for_asr[DATA_SIZE];

#endif
