/*---------------------------------------------------------------------------
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include "public.h"
#include <stdint.h>
#include <string.h>

int32_t arm_beamformer_f32(int32_t, void **, void *, void *);
int32_t xiph_libspeex_aec_f32(int32_t, void **, void *, void *);
int32_t xiph_libspeex_anr_f32(int32_t, void **, void *, void *);
int32_t ee_kws_f32(int32_t, void **, void *, void *);

#define DATA_SIZE (211 * 128)
const int16_t downlink_audio[DATA_SIZE] = {
#include "../application_demo/debug_data/noise.txt"
};
const int16_t left_microphone_capture[DATA_SIZE] = {
#include "../application_demo/debug_data/left0.txt"
};
const int16_t right_microphone_capture[DATA_SIZE] = {
#include "../application_demo/debug_data/right0.txt"
};

static int16_t for_asr[DATA_SIZE];

/* These are index pointers used to slide through the input audio stream. */
static uint32_t idx_microphone_L;
static uint32_t idx_microphone_R;
static uint32_t idx_downlink;
static uint32_t idx_for_asr;

#define AUDIO_CAPTURE_SAMPLING    16000
#define AUDIO_CAPTURE_FRAME_LEN_S 0.016
#define MONO                      1
#define SAMPLE_SIZE               2 // int16
/* #define AUDIO_NB_SAMPLES \
    (const int)(AUDIO_CAPTURE_SAMPLING * AUDIO_CAPTURE_FRAME_LEN_S) */
#define AUDIO_NB_SAMPLES 256
#define N                (AUDIO_NB_SAMPLES * SAMPLE_SIZE)

static int16_t audio_input[AUDIO_NB_SAMPLES];       // 1
static int16_t left_capture[AUDIO_NB_SAMPLES];      // 2
static int16_t right_capture[AUDIO_NB_SAMPLES];     // 3
static int16_t beamformer_output[AUDIO_NB_SAMPLES]; // 4
static int16_t aec_output[AUDIO_NB_SAMPLES];        // 5
static int16_t audio_fifo[13 * 64];        // 6 ptorelli FIXME TODO
static int8_t mfcc_fifo[490];         // 7 ptorelli FIXME TODO
static int8_t classes[12];           // 8 ptorelli FIXME TODO

/* The above buffers are programmed into these XDAIS structures on init. */
static xdais_buffer_t xdais_bmf[3];
static xdais_buffer_t xdais_aec[3];
static xdais_buffer_t xdais_anr[2];
static xdais_buffer_t xdais_kws[4];

// instances of the components
static uint32_t *p_bmf_inst;
static uint32_t *p_aec_inst;
static uint32_t *p_anr_inst;
static uint32_t *p_kws_inst;

// TODO: ptorelli: None of the memreq's use memory; only four instances
#define MAX_ALLOC_WORDS 20 /* 28500 */
static uint32_t all_instances[MAX_ALLOC_WORDS], idx_malloc;

uint32_t parameters[1]; // pre-computed parameter index

void
reset_audio(void)
{
    idx_downlink     = 0;
    idx_microphone_L = 0;
    idx_microphone_R = 0;
    idx_for_asr      = 0;
}

int
copy_audio(int16_t *pt, int16_t debug)
{
    uint32_t *     idx = NULL;
    const int16_t *src = NULL;
    int16_t *      dst = NULL;

    if (debug > 0)
    {
        return 0;
    }

    if (pt == audio_input)
    {
        idx = &idx_downlink;
        src = &(downlink_audio[*idx]);
        dst = audio_input;
    }
    else if (pt == left_capture)
    {
        idx = &idx_microphone_L;
        src = &(left_microphone_capture[*idx]);
        dst = left_capture;
    }
    else if (pt == right_capture)
    {
        idx = &idx_microphone_R;
        src = &(right_microphone_capture[*idx]);
        dst = right_capture;
    }
    else if (pt == aec_output)
    {
        idx = &idx_for_asr;
        src = aec_output;
        dst = &(for_asr[*idx]);
    }
    else
    {
        return 1;
    }

    if (idx_downlink + (N / 2) >= DATA_SIZE)
    {
        return 1;
    }

    if (src != 0)
    {
        memcpy(dst, src, AUDIO_NB_SAMPLES * SAMPLE_SIZE * MONO);
    }

    *idx += (N / 2);

    return 0;
}

#define SETUP_XDAIS(SRC, DATA, SIZE) \
    {                                \
        SRC.p_data = (PTR_INT)DATA;  \
        SRC.size   = SIZE;           \
    }

#define CALL_MEMREQ(FUNC, REQ, PARAMS)                 \
    {                                                  \
        uint32_t *p_req = REQ;                         \
        FUNC(NODE_MEMREQ, (void **)&p_req, 0, PARAMS); \
    }

// TODO: ptorelli: this assumes all pointers are uint32_t bytes. FIXME.
// TODO: ptorelli: why is req / 4?
#define LOCAL_ALLOC(PINST, REQ)               \
    {                                         \
        PINST = &(all_instances[idx_malloc]); \
        idx_malloc += 1 + REQ / 4;            \
    }

void
audiomark_initialize(void)
{
    uint32_t memreq_bmf_f32[1]; // memreq will grow later
    uint32_t memreq_aec_f32[1];
    uint32_t memreq_anr_f32[1];
    uint32_t memreq_kws_f32[1];

    SETUP_XDAIS(xdais_bmf[0], left_capture, N);
    SETUP_XDAIS(xdais_bmf[1], right_capture, N);
    SETUP_XDAIS(xdais_bmf[2], beamformer_output, N);

    SETUP_XDAIS(xdais_aec[0], beamformer_output, N);
    SETUP_XDAIS(xdais_aec[1], audio_input, N);
    SETUP_XDAIS(xdais_aec[2], aec_output, N);

    SETUP_XDAIS(xdais_anr[0], aec_output, N);
    SETUP_XDAIS(xdais_anr[1], aec_output, N); /* output overwrites input */

    SETUP_XDAIS(xdais_kws[0], aec_output, N);
    SETUP_XDAIS(xdais_kws[1], audio_fifo, 13 * 64 * 2); // ptorelli: fixme
    SETUP_XDAIS(xdais_kws[2], mfcc_fifo, 490);  // ptorelli: fixme
    SETUP_XDAIS(xdais_kws[3], classes, 12);    // ptorelli: fixme

    parameters[0] = 0; // take the first set of parameters

    /* Call the components for their memory requests. */
    CALL_MEMREQ(arm_beamformer_f32, memreq_bmf_f32, parameters);
    CALL_MEMREQ(xiph_libspeex_aec_f32, memreq_aec_f32, parameters);
    CALL_MEMREQ(xiph_libspeex_anr_f32, memreq_anr_f32, parameters);
    CALL_MEMREQ(ee_kws_f32, memreq_kws_f32, parameters);

    /* Using our heap `all_instances` assign the instances and requests */
    LOCAL_ALLOC(p_bmf_inst, memreq_bmf_f32[0]);
    LOCAL_ALLOC(p_aec_inst, memreq_aec_f32[0]);
    LOCAL_ALLOC(p_anr_inst, memreq_anr_f32[0]);
    LOCAL_ALLOC(p_kws_inst, memreq_kws_f32[0]);

    if (idx_malloc >= MAX_ALLOC_WORDS)
    {
        // TODO: ptorelli: portable error handler
        while (1)
        {
        };
    }

    arm_beamformer_f32(NODE_RESET, (void **)&p_bmf_inst, 0, parameters);
    xiph_libspeex_aec_f32(NODE_RESET, (void **)&p_aec_inst, 0, parameters);
    xiph_libspeex_anr_f32(NODE_RESET, (void **)&p_anr_inst, 0, parameters);
    ee_kws_f32(NODE_RESET, (void **)&p_kws_inst, 0, parameters);
}

#define TEST_BF  0 // beamformer test
#define TEST_AEC 0 // acoustic echo canceller test
#define TEST_ANR 0 // adaptive noise reduction test
#define TESTS    (TEST_BF + TEST_AEC + TEST_ANR)

#define ERR_BREAK(X) \
    if (X == 1)      \
    {                \
        break;       \
    }

void
audiomark_run(void)
{
    for (int j = 0; j < 5; ++j)
    {
        reset_audio();
        while (1)
        {
            ERR_BREAK(copy_audio(audio_input, 0));
            ERR_BREAK(copy_audio(left_capture, 0));
            ERR_BREAK(copy_audio(right_capture, 0));

#if TESTS == 0
            // linear feedback of the loudspeaker to the MICs
            for (int i = 0; i < N / 2; i++)
            {
                left_capture[i]  = left_capture[i] + audio_input[i];
                right_capture[i] = right_capture[i] + audio_input[i];
            }

            // TODO: ptorelli: return status should be checked!
            arm_beamformer_f32(NODE_RUN, (void *)&p_bmf_inst, xdais_bmf, 0);
            xiph_libspeex_aec_f32(NODE_RUN, (void *)&p_aec_inst, xdais_aec, 0);
            xiph_libspeex_anr_f32(NODE_RUN, (void *)&p_anr_inst, xdais_anr, 0);
            ee_kws_f32(NODE_RUN, (void *)&p_kws_inst, xdais_kws, 0);

#else
#if TEST_BF
            arm_beamformer_f32(NODE_RUN, (void *)&p_bmf_inst, xdais_bmf, 0);
            memmove(aec_output, beamformer_output, N);
#endif

#if TEST_AEC
            // linear feedback of the loudspeaker to the MICs
            for (i = 0; i < N / 2; i++)
            {
                left_capture[i]  = left_capture[i] + audio_input[i];
                right_capture[i] = right_capture[i] + audio_input[i];
            }
            memmove(beamformer_output, left_capture, N);
            xiph_libspeex_aec_f32(NODE_RUN, (void *)&p_aec_inst, xdais_aec, 0);
#endif

#if TEST_ANR
            memmove(aec_output, audio_input, N);
            xiph_libspeex_anr_f32(NODE_RUN, (void *)&p_anr_inst, xdais_anr, 0);
#endif
#endif
            // save the cleaned audio for ASR
            ERR_BREAK(copy_audio(aec_output, 0));
        }
    }
}
