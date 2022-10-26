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

#ifdef _MSC_VER 
#include <stdio.h>
#pragma warning(disable : 4996)
#endif

 
  // Graph of 3 components with buffer numbers given in parenthesis
  //
  //+-----------(1)----------------------------------------------------------->
  //                                   |
  //                                   |
  //                                   |
  //          +--------+           +---v----+             +--------+
  //<---(5)---+  ANR   +<---(5)----+ AEC    +<-----(4)----+  BF     <==(2,3)===
  //          +--------+           +--------+             +--------+



extern int32_t arm_beamformer_f32    (int32_t command, void **instance, void *data, void *parameters);
extern int32_t xiph_libspeex_aec_f32 (int32_t command, void **instance, void *data, void *parameters);
extern int32_t xiph_libspeex_anr_f32 (int32_t command, void **instance, void *data, void *parameters);
extern int32_t ee_kws_f32            (int32_t command, void **instance, void *data, void *parameters);

#ifdef _MSC_VER 
#else
#define DATA_SIZE (211*128)
const int16_t downlink_audio [DATA_SIZE] = {
    #include "../application_demo/debug_data/noise.txt"
};
const int16_t left_microphone_capture [DATA_SIZE] = {
    #include "../application_demo/debug_data/left0.txt"
};
const int16_t right_microphone_capture [DATA_SIZE] = {
    #include "../application_demo/debug_data/right0.txt"
};

static int16_t loudspeaker [DATA_SIZE];
static int16_t for_asr [DATA_SIZE];
#endif


static uint32_t idx_microphone_L, idx_microphone_R, idx_downlink, idx_loudspeaker, idx_for_asr, global_no_error;

#define AUDIO_CAPTURE_SAMPLING 16000
#define AUDIO_CAPTURE_FRAME_LEN_S 0.016
#define MONO 1
#define STEREO 2
#define SAMPLE_SIZE 2 //int16
#define AUDIO_CAPTURE_SIZE (int)(AUDIO_CAPTURE_SAMPLING * AUDIO_CAPTURE_FRAME_LEN_S * MONO * 2) /* deinterleaved MONO */
#define AUDIO_NB_SAMPLES (const int)(AUDIO_CAPTURE_SAMPLING * AUDIO_CAPTURE_FRAME_LEN_S)
#define N (AUDIO_NB_SAMPLES * SAMPLE_SIZE)

static int16_t audio_input       [AUDIO_NB_SAMPLES]; //1     
static int16_t left_capture      [AUDIO_NB_SAMPLES]; //2
static int16_t right_capture     [AUDIO_NB_SAMPLES]; //3
static int16_t beamformer_output [AUDIO_NB_SAMPLES]; //4
static int16_t aec_output        [AUDIO_NB_SAMPLES]; //5
static int16_t audio_fifo        [AUDIO_NB_SAMPLES]; //6
static int16_t mfcc_fifo         [AUDIO_NB_SAMPLES]; //7
static int16_t classes           [AUDIO_NB_SAMPLES]; //8

// instances of the components
static int32_t *pt_BF_f32_instance;
static int32_t *pt_aec_f32_instance;
static int32_t *pt_anr_f32_instance;
static int32_t *pt_kws_f32_instance;

#define MAX_ALLOC_WORDS 28500
static uint32_t all_instances[MAX_ALLOC_WORDS], idx_malloc;

// for libspeex malloc() 
char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
uint32_t cumulatedMalloc;

// pointer to the buffers and parameters
uint32_t *prq;
uint32_t memreq_BF_f32 [1];   // memreq will grow later
uint32_t memreq_aec_f32[1];
uint32_t memreq_anr_f32[1];
uint32_t memreq_kws_f32[1]; // TODO: ptorelli: what is this?

// TODO: ptorelli: why are these 2x the size, i.e., 6/6/4 when we only use 3/3/2?
data_buffer_t data_BF_f32  [6];    // XDAIS format
data_buffer_t data_aec_f32 [6];
data_buffer_t data_anr_f32 [4];
data_buffer_t data_kws_f32 [8];

uint32_t parameters[1];       // pre-computed parameter index 

#ifdef _MSC_VER 
#define AUDIO_INPUT "../../../application_demo/debug_data/Noise.raw"
#define AUDIO_OUTPUT "../../../application_demo/debug_data/asr_input.raw"
#define AUDIO_CAPTURE_FILE_NAME_LEFT "../../../application_demo/debug_data/left0.raw"
#define AUDIO_CAPTURE_FILE_NAME_RIGHT "../../../application_demo/debug_data/right0.raw"
#define DEBUG_TRACE "trace.txt"
FILE *ptf_mic_L, *ptf_mic_R;
FILE *ptf_audio_input, *ptf_to_asr;
#endif


/*
 * -----------------------------------------------------------------------
 */
void copy_audio (int16_t * pt, int16_t debug)
{ 
#ifdef _MSC_VER 
    size_t n;
    if (debug == 0) 
    {   if (pt == audio_input) { if(N != (n=fread (audio_input, 1, N, ptf_audio_input))) global_no_error=0;};
        if (pt == left_capture) { if(N != (n=fread (left_capture, 1, N, ptf_mic_L))) global_no_error=0;};
        if (pt == right_capture) { if(N != (n=fread (right_capture, 1, N, ptf_mic_R))) global_no_error=0;};
        if (pt == aec_output) { n = fwrite(aec_output, 1, N, ptf_to_asr); };
    } 

#else
    uint32_t n, *idx; uint8_t *src=0, *dst;
    if (debug > 0) return;
    if (pt == audio_input) { idx=&idx_downlink; src=&(downlink_audio[*idx]); dst=audio_input; };
    if (pt == left_capture) { idx=&idx_microphone_L; src=&(left_microphone_capture[*idx]); dst=left_capture;};
    if (pt == right_capture) { idx=&idx_microphone_R; src=&(right_microphone_capture[*idx]); dst=right_capture; };
    if (pt == aec_output) { idx=&idx_for_asr; src=aec_output; dst=&(for_asr[*idx]); };

    if (idx_downlink + (N/2) >= DATA_SIZE)
    {   global_no_error = 0; return;
    }
    if (src!=0) memcpy (dst, src, AUDIO_NB_SAMPLES * SAMPLE_SIZE * MONO);
    *idx += (N/2); 
#endif
}

/*---------------------------------------------------------------------------
 * Application initialization
 * 
 * components' parameters are a list of pairs [pointer, size] for inputs
 *   followed by similar pairs for outputs [*,n]
 *---------------------------------------------------------------------------*/
void audiomark_initialize(void)
{
    // "XDAIS"-like pointer structures
    // Beamformer : two pairs [*,n] for microphone inputs and one pair [*,n] for output
    data_BF_f32 [0].data_struct[0] = (PTR_INT)left_capture;  data_BF_f32[0].data_struct[1] = N;
    data_BF_f32 [1].data_struct[0] = (PTR_INT)right_capture;  data_BF_f32[1].data_struct[1] = N;
    data_BF_f32 [2].data_struct[0] = (PTR_INT)beamformer_output;  data_BF_f32[2].data_struct[1] = N;

    // AEC : two pairs [*,n] for Reference and Microphone inputs and one pair [*,n] for output
    data_aec_f32[0].data_struct[0] = (PTR_INT)beamformer_output; data_aec_f32[0].data_struct[1] = N; // reference "in"
    data_aec_f32[1].data_struct[0] = (PTR_INT)audio_input; data_aec_f32[1].data_struct[1] = N; // echo "far_end"
    data_aec_f32[2].data_struct[0] = (PTR_INT)aec_output; data_aec_f32[2].data_struct[1] = N; // outbuf "out"
    
    // ANR : one pair [*,n] for mono input and one pair [*,n] for output
    data_anr_f32[0].data_struct[0] = (PTR_INT)aec_output; data_anr_f32[0].data_struct[1] = N;
    /* TODO: ptorelli: it appears ANR output over-writes its input, correct? */
    data_anr_f32[1].data_struct[0] = (PTR_INT)aec_output; data_anr_f32[1].data_struct[1] = N;

    // KWS
    data_kws_f32[0].data_struct[0] = (PTR_INT)aec_output; data_kws_f32[0].data_struct[1] = N;
    data_kws_f32[1].data_struct[0] = (PTR_INT)audio_fifo; data_kws_f32[1].data_struct[1] = N; // TODO: not N, but we are in dev mode
    data_kws_f32[2].data_struct[0] = (PTR_INT)mfcc_fifo;  data_kws_f32[2].data_struct[1] = N; // TODO: not N, but we are in dev mode
    data_kws_f32[3].data_struct[0] = (PTR_INT)classes;    data_kws_f32[3].data_struct[1] = N; // TODO: not N, but we are in dev mode

    parameters[0] = 0;   // take the first set of parameters

    // instances memory allocations
    prq = memreq_BF_f32;  arm_beamformer_f32    (NODE_MEMREQ, (void *)&prq, 0, parameters);
    prq = memreq_aec_f32; xiph_libspeex_aec_f32 (NODE_MEMREQ, (void *)&prq, 0, parameters);
    prq = memreq_anr_f32; xiph_libspeex_anr_f32 (NODE_MEMREQ, (void *)&prq, 0, parameters);
    prq = memreq_kws_f32; ee_kws_f32            (NODE_MEMREQ, (void *)&prq, 0, parameters);

    // TODO: ptorelli: clarify what is going on here.
    pt_BF_f32_instance  = &(all_instances[idx_malloc]); idx_malloc += 1+memreq_BF_f32[0]/4;
    pt_aec_f32_instance = &(all_instances[idx_malloc]); idx_malloc += 1+memreq_aec_f32[0]/4;
    pt_anr_f32_instance = &(all_instances[idx_malloc]); idx_malloc += 1+memreq_anr_f32[0]/4;
    pt_kws_f32_instance = &(all_instances[idx_malloc]); idx_malloc += 1+memreq_kws_f32[0]/4;
    if (idx_malloc >= MAX_ALLOC_WORDS)
    {
        while (1) {};
    }

    arm_beamformer_f32    (NODE_RESET, (void *)&pt_BF_f32_instance,  0, parameters);
    xiph_libspeex_aec_f32 (NODE_RESET, (void *)&pt_aec_f32_instance, 0, parameters);
    xiph_libspeex_anr_f32 (NODE_RESET, (void *)&pt_anr_f32_instance, 0, parameters);
    ee_kws_f32            (NODE_RESET, (void *)&pt_kws_f32_instance, 0, parameters);

    global_no_error = 1;

#ifdef _MSC_VER 
    ptf_audio_input = fopen (AUDIO_INPUT, "rb");             
    ptf_mic_L = fopen (AUDIO_CAPTURE_FILE_NAME_LEFT, "rb"); 
    ptf_mic_R = fopen (AUDIO_CAPTURE_FILE_NAME_RIGHT, "rb"); 
    ptf_to_asr = fopen (AUDIO_OUTPUT, "wb");
#endif 
}

/*---------------------------------------------------------------------------
 * Application execution
 *---------------------------------------------------------------------------*/

void audiomark_run(void)
{
    uint32_t i;
  

  // ------------audio_input (from application processor)---------------------TO LOUDSPEAKER------------->
  //                                      |
  //                                      |
  //                                      |
  //           +--------+             +---v----+                        +--------+
  // <--TO ASR-+  ANR   +<-aec_output-+ AEC    +<---beamformer_output---+  BF    +<==left/right_capture===
  //           +--------+             +--------+                        +--------+

	do 
    {   // read audio and MIC data, N=number of bytes per mono audio packet
        copy_audio (audio_input, 0);    
        copy_audio (left_capture, 0);   
        copy_audio (right_capture, 0);  

#define TEST_BF  0  // beamformer test
#define TEST_AEC 0  // acoustic echo canceller test
#define TEST_ANR 0  // adaptive noise reduction test
#define TESTS (TEST_BF + TEST_AEC + TEST_ANR)

#if TESTS == 0
        // linear feedback of the loudspeaker to the MICs
        for (i=0; i<N/2; i++)
        {   
            left_capture[i] = left_capture[i] + audio_input[i]; 
            right_capture[i] = right_capture[i] + audio_input[i];
        }
        arm_beamformer_f32    (NODE_RUN, (void *)&pt_BF_f32_instance, data_BF_f32, 0);
        xiph_libspeex_aec_f32 (NODE_RUN, (void *)&pt_aec_f32_instance, data_aec_f32, 0);
        xiph_libspeex_anr_f32 (NODE_RUN, (void *)&pt_anr_f32_instance, data_anr_f32, 0);
        ee_kws_f32            (NODE_RUN, (void *)&pt_kws_f32_instance, data_kws_f32, 0);

#else
    #if TEST_BF 
        arm_beamformer_f32    (NODE_RUN, (void *)&pt_BF_f32_instance, data_BF_f32, 0);
        memmove(aec_output, beamformer_output, N);
    #endif 

    #if TEST_AEC
        // linear feedback of the loudspeaker to the MICs
        for (i=0; i<N/2; i++)
        {   
            left_capture[i] = left_capture[i] + audio_input[i]; 
            right_capture[i] = right_capture[i] + audio_input[i];
        }
        memmove(beamformer_output, left_capture, N);
        xiph_libspeex_aec_f32 (NODE_RUN, (void *)&pt_aec_f32_instance, data_aec_f32, 0);
    #endif

    #if TEST_ANR
        memmove(aec_output, audio_input, N);
        xiph_libspeex_anr_f32 (NODE_RUN, (void *)&pt_anr_f32_instance, data_anr_f32, 0);
    #endif
#endif

        // save the cleaned audio for ASR
        copy_audio (aec_output, 0);

    }   while (global_no_error);

#ifdef _MSC_VER 
    fclose(ptf_audio_input);
    fclose(ptf_mic_L);
    fclose(ptf_mic_R);
    fclose(ptf_to_asr);
#endif 
}

/*--------------------------------------------------------------------------- */
