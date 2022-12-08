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

#include "ee_audiomark.h"

extern const int16_t downlink_audio[NINPUT_SAMPLES];
extern const int16_t left_microphone_capture[NINPUT_SAMPLES];
extern const int16_t right_microphone_capture[NINPUT_SAMPLES];
extern int16_t       for_asr[NINPUT_SAMPLES];
// System integrator can locate these via the linker map (th_api.c)
extern int16_t audio_input[SAMPLES_PER_AUDIO_FRAME];       // 1
extern int16_t left_capture[SAMPLES_PER_AUDIO_FRAME];      // 2
extern int16_t right_capture[SAMPLES_PER_AUDIO_FRAME];     // 3
extern int16_t beamformer_output[SAMPLES_PER_AUDIO_FRAME]; // 4
extern int16_t aec_output[SAMPLES_PER_AUDIO_FRAME];        // 5
extern int16_t audio_fifo[AUDIO_FIFO_SAMPLES];             // 6
extern int8_t  mfcc_fifo[MFCC_FIFO_BYTES];                 // 7
extern int8_t  classes[OUT_DIM];                           // 8

/* These are index pointers used to slide through the input audio stream. */
static uint32_t idx_microphone_L;
static uint32_t idx_microphone_R;
static uint32_t idx_downlink;
static uint32_t idx_for_asr;
static uint32_t progress_count;

// These are used by Speex's internal speex_alloc function for custom heaps.
char *spxGlobalHeapPtr;
char *spxGlobalHeapEnd;
long  cumulatedMalloc;

/* The above buffers are programmed into these XDAIS structures on init. */
static xdais_buffer_t xdais_bmf[3];
static xdais_buffer_t xdais_aec[3];
static xdais_buffer_t xdais_anr[2];
static xdais_buffer_t xdais_kws[4];

static void *p_bmf_inst;
static void *p_aec_inst;
static void *p_anr_inst;
static void *p_kws_inst;

static int read_all_audio_data = 0;

void
reset_audio(void)
{
    idx_downlink        = 0;
    idx_microphone_L    = 0;
    idx_microphone_R    = 0;
    idx_for_asr         = 0;
    read_all_audio_data = 0;
    progress_count      = 0;
}

int
copy_audio(int16_t *pt, int16_t debug)
{
    uint32_t      *idx = NULL;
    const int16_t *src = NULL;
    int16_t       *dst = NULL;

    if (debug > 0)
    {
        return 0;
    }

    if (pt == audio_input)
    {
        idx = &idx_downlink;
        src = &(downlink_audio[*idx]);
        dst = audio_input;
        // Only need to increment this once since they all move together
        progress_count += SAMPLES_PER_AUDIO_FRAME;
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

    if ((progress_count + SAMPLES_PER_AUDIO_FRAME) >= NINPUT_SAMPLES)
    {
        read_all_audio_data = 1;
        return 1;
    }

    if (src != 0)
    {
        th_memcpy(dst, src, BYTES_PER_AUDIO_FRAME);
    }

    *idx += SAMPLES_PER_AUDIO_FRAME;

    return 0;
}

int
audiomark_initialize(void)
{
    // For dereferencing
    uint32_t *p_req;

    uint32_t memreq_bmf_f32;
    uint32_t memreq_aec_f32;
    uint32_t memreq_anr_f32;
    uint32_t memreq_kws_f32;

    uint32_t param_idx = 0;

    SETUP_XDAIS(xdais_bmf[0], left_capture, BYTES_PER_AUDIO_FRAME);
    SETUP_XDAIS(xdais_bmf[1], right_capture, BYTES_PER_AUDIO_FRAME);
    SETUP_XDAIS(xdais_bmf[2], beamformer_output, BYTES_PER_AUDIO_FRAME);

    SETUP_XDAIS(xdais_aec[0], beamformer_output, BYTES_PER_AUDIO_FRAME);
    SETUP_XDAIS(xdais_aec[1], audio_input, BYTES_PER_AUDIO_FRAME);
    SETUP_XDAIS(xdais_aec[2], aec_output, BYTES_PER_AUDIO_FRAME);

    SETUP_XDAIS(xdais_anr[0], aec_output, BYTES_PER_AUDIO_FRAME);
    // N.B.: Output overwrites input.
    SETUP_XDAIS(xdais_anr[1], aec_output, BYTES_PER_AUDIO_FRAME);

    SETUP_XDAIS(xdais_kws[0], aec_output, BYTES_PER_AUDIO_FRAME);
    SETUP_XDAIS(xdais_kws[1], audio_fifo, AUDIO_FIFO_SAMPLES * 2);
    SETUP_XDAIS(xdais_kws[2], mfcc_fifo, MFCC_FIFO_BYTES);
    SETUP_XDAIS(xdais_kws[3], classes, OUT_DIM);

    /* Call the components for their memory requests. */
    p_req = &memreq_bmf_f32;
    ee_abf_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);
    p_req = &memreq_aec_f32;
    ee_aec_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);
    p_req = &memreq_anr_f32;
    ee_anr_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);
    p_req = &memreq_kws_f32;
    ee_kws_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);

    /*
        printf("Memory alloc summary:\n");
        printf(" bmf = %d\n", memreq_bmf_f32);
        printf(" aec = %d\n", memreq_aec_f32);
        printf(" anr = %d\n", memreq_anr_f32);
        printf(" kws = %d\n", memreq_kws_f32);
    */

    /* Using our heap `all_instances` assign the instances and requests */
    p_bmf_inst = th_malloc(memreq_bmf_f32, COMPONENT_BMF);
    p_aec_inst = th_malloc(memreq_aec_f32, COMPONENT_AEC);
    p_anr_inst = th_malloc(memreq_anr_f32, COMPONENT_ANR);
    // This does not allocate the neural net memory, see th_api.c
    p_kws_inst = th_malloc(memreq_kws_f32, COMPONENT_KWS);

    if (!p_bmf_inst || !p_aec_inst || !p_anr_inst || !p_kws_inst)
    {
        // printf("Out of heap memory\n");
        return 1;
    }

    ee_abf_f32(NODE_RESET, (void **)&p_bmf_inst, 0, NULL);
    ee_aec_f32(NODE_RESET, (void **)&p_aec_inst, 0, &param_idx);
    ee_anr_f32(NODE_RESET, (void **)&p_anr_inst, 0, &param_idx);
    ee_kws_f32(NODE_RESET, (void **)&p_kws_inst, 0, NULL);

    return 0;
}

void
audiomark_release(void)
{
    th_free(p_bmf_inst, COMPONENT_BMF);
    th_free(p_aec_inst, COMPONENT_AEC);
    th_free(p_anr_inst, COMPONENT_ANR);
    th_free(p_kws_inst, COMPONENT_KWS);
}

#define CHECK(X)         \
    if (X == 1)          \
    {                    \
        goto exit_error; \
    }

int
audiomark_run(void)
{
    for (int j = 0; j < 1; ++j)
    {
        reset_audio();
        while (!read_all_audio_data)
        {
            copy_audio(audio_input, 0);
            copy_audio(left_capture, 0);
            copy_audio(right_capture, 0);

            // linear feedback of the loudspeaker to the MICs
            for (int i = 0; i < BYTES_PER_AUDIO_FRAME / 2; i++)
            {
                left_capture[i]  = left_capture[i] + audio_input[i];
                right_capture[i] = right_capture[i] + audio_input[i];
            }

            CHECK(ee_abf_f32(NODE_RUN, (void **)&p_bmf_inst, xdais_bmf, NULL));
            CHECK(ee_aec_f32(NODE_RUN, (void **)&p_aec_inst, xdais_aec, NULL));
            CHECK(ee_anr_f32(NODE_RUN, (void **)&p_anr_inst, xdais_anr, NULL));
            CHECK(ee_kws_f32(NODE_RUN, (void **)&p_kws_inst, xdais_kws, NULL));

            // save the cleaned audio for ASR
            copy_audio(aec_output, 0);
        }
    }
    return 0;
exit_error:
    return -1;
}
