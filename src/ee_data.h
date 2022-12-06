#ifndef __EE_DATA_H
#define __EE_DATA_H

#define NINPUT_SAMPLES (211 * 128)

const int16_t downlink_audio[NINPUT_SAMPLES] = {
#include "ee_data/noise.txt"
};
const int16_t left_microphone_capture[NINPUT_SAMPLES] = {
#include "ee_data/left0.txt"
};
const int16_t right_microphone_capture[NINPUT_SAMPLES] = {
#include "ee_data/right0.txt"
};

static int16_t for_asr[NINPUT_SAMPLES];

#endif
