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

#include "../../application_demo/public.h"
#include <stdint.h>
#include <string.h>

#ifdef _MSC_VER 
#include <stdio.h>
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


#ifdef _MSC_VER 
#else
#define DATA_SIZE (211*128)
const int16_t downlink_audio2 [DATA_SIZE] = {
    #include "../../application_demo/debug_data/noise.txt"
};
const int16_t left_microphone_capture2 [DATA_SIZE] = {
    #include "../../application_demo/debug_data/left2.txt"
};
const int16_t right_microphone_capture2 [DATA_SIZE] = {
    #include "../../application_demo/debug_data/right2.txt"
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

static int16_t input_audio       [AUDIO_NB_SAMPLES]; //1     
static int16_t left_capture      [AUDIO_NB_SAMPLES]; //2
static int16_t right_capture     [AUDIO_NB_SAMPLES]; //3
static int16_t beamformer_output [AUDIO_NB_SAMPLES]; //4
static int16_t aec_output        [AUDIO_NB_SAMPLES]; //5

// instances of the components
static int32_t *pt_BF_f32_instance;
static int32_t *pt_aec_f32_instance;
static int32_t *pt_anr_f32_instance;

static uint32_t all_instances[24000], idx_malloc;

// for libspeex malloc() 
char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
uint32_t cumulatedMalloc;

// pointer to the buffers and parameters
uint32_t *prq;
uint32_t memreq_BF_f32 [1];   // memreq will grow later
uint32_t memreq_aec_f32[1];
uint32_t memreq_anr_f32[1];

// assuming "int" is also the same size as "*int"
#define PTR_INT uint32_t
typedef struct {
  PTR_INT data_struct[2];
} data_buffer_t;

//#define PTR_INT uint64_t
//typedef struct {
//  uint64_t data_struct[2];
//} data_buffer_t;
//#define PTR_INT int
//typedef struct {
//  int data_struct[2];
//} data_buffer_t;


data_buffer_t data_BF_f32  [6];    // XDAIS format
data_buffer_t data_aec_f32 [6];
data_buffer_t data_anr_f32 [4];

uint32_t parameters[1];       // pre-computed parameter index 

// just to ease code reading 
int16_t *buf1, *buf2, *buf3, *buf4, *buf5;

#ifdef _MSC_VER 
#define AUDIO_INPUT "../../../application_demo/debug_data/Noise.raw"
#define AUDIO_OUTPUT "../../../application_demo/debug_data/asr_input.raw"
#define AUDIO_CAPTURE_FILE_NAME_LEFT "../../../application_demo/debug_data/left0.raw"
#define AUDIO_CAPTURE_FILE_NAME_RIGHT "../../../application_demo/debug_data/right0.raw"
FILE *ptf_mic_L, *ptf_mic_R;
FILE *ptf_input_audio, *ptf_to_asr;
#endif


/*
 * -----------------------------------------------------------------------
 */
void copy_audio (int16_t * pt)
{ 
#ifdef _MSC_VER 
    uint32_t n;
    if (pt == buf1) { if(N != (n=fread (buf1, 1, N, ptf_input_audio)))      global_no_error=0;};
    if (pt == buf2) { if(N != (n=fread (buf2, 1, N, ptf_mic_L))) global_no_error=0;};
    if (pt == buf3) { if(N != (n=fread (buf3, 1, N, ptf_mic_R)))global_no_error=0;};
    if (pt == buf5) { n = fwrite(buf5, 1, N, ptf_to_asr); };
#else
    uint32_t n, *idx; uint8_t *src, *dst;
    if (pt == buf1) { idx=&idx_downlink; src=downlink_audio2; dst=buf1; };
    if (pt == buf2) { idx=&idx_microphone_L; src=&(left_microphone_capture2[*idx]); dst=buf2; };
    if (pt == buf3) { idx=&idx_microphone_R; src=&(right_microphone_capture2[*idx]); dst=buf3; };
    if (pt == buf5) { idx=&idx_for_asr; src=buf5; dst=&(for_asr[*idx]); };

    if (*idx + AUDIO_NB_SAMPLES >= DATA_SIZE)
    {   global_no_error = 0; return;
    }
    memcpy (dst, src, AUDIO_NB_SAMPLES * SAMPLE_SIZE * MONO);
    *idx += AUDIO_NB_SAMPLES;
#endif
}

/*---------------------------------------------------------------------------
 * Application initialization
 *---------------------------------------------------------------------------*/
void hard_coded_demo_initialize(void)
{
    buf1 = input_audio;
    buf2 = left_capture;
    buf3 = right_capture;
    buf4 = beamformer_output;
    buf5 = aec_output;

    // "XDAIS"-like pointer structures
    data_BF_f32 [0].data_struct[0] = (PTR_INT)buf2;  data_BF_f32[0].data_struct[1] = N;
    data_BF_f32 [1].data_struct[0] = (PTR_INT)buf3;  data_BF_f32[1].data_struct[1] = N;
    data_BF_f32 [2].data_struct[0] = (PTR_INT)buf4;  data_BF_f32[2].data_struct[1] = N;
    data_aec_f32[0].data_struct[0] = (PTR_INT)buf4; data_aec_f32[0].data_struct[1] = N;
    data_aec_f32[1].data_struct[0] = (PTR_INT)buf1; data_aec_f32[1].data_struct[1] = N;
    data_aec_f32[2].data_struct[0] = (PTR_INT)buf5; data_aec_f32[2].data_struct[1] = N;
    data_anr_f32[0].data_struct[0] = (PTR_INT)buf5; data_anr_f32[0].data_struct[1] = N;
    data_anr_f32[1].data_struct[0] = (PTR_INT)buf5; data_anr_f32[1].data_struct[1] = N;

    parameters[0] = 0;   // take the first set of parameters


    //param_anr_f32[0] = 256;   // NN
    //param_anr_f32[1] = 16000; // FS

    // instances initialization and memory allocations
    prq = memreq_BF_f32;  arm_beamformer_f32    (_NODE_MEMREQ, (void *)&prq, 0, parameters);
    prq = memreq_aec_f32; xiph_libspeex_aec_f32 (_NODE_MEMREQ, (void *)&prq, 0, parameters);
    prq = memreq_anr_f32; xiph_libspeex_anr_f32 (_NODE_MEMREQ, (void *)&prq, 0, parameters);

    #define M 8 // margin 
    pt_BF_f32_instance  = &(all_instances[idx_malloc]); idx_malloc += M + memreq_BF_f32[0]/4;
    pt_aec_f32_instance = &(all_instances[idx_malloc]); idx_malloc += M + memreq_aec_f32[0]/4;
    pt_anr_f32_instance = &(all_instances[idx_malloc]); idx_malloc += M + memreq_anr_f32[0]/4;

    arm_beamformer_f32    (_NODE_RESET, (void *)&pt_BF_f32_instance,  0, parameters);
    xiph_libspeex_aec_f32 (_NODE_RESET, (void *)&pt_aec_f32_instance, 0, parameters);
    xiph_libspeex_anr_f32 (_NODE_RESET, (void *)&pt_anr_f32_instance, 0, parameters);

    global_no_error = 1;

#ifdef _MSC_VER 
    ptf_input_audio = fopen (AUDIO_INPUT, "rb");    // downlink audio (1)
    ptf_mic_L  = fopen (AUDIO_CAPTURE_FILE_NAME_LEFT, "rb");  // (2)
    ptf_mic_R = fopen (AUDIO_CAPTURE_FILE_NAME_RIGHT, "rb"); // (3)
    ptf_to_asr = fopen (AUDIO_OUTPUT, "wb");                  // (5)
#endif 
}

/*---------------------------------------------------------------------------
 * Application execution
 *---------------------------------------------------------------------------*/
void hard_coded_demo_run(void)
{
    uint32_t i;

  //+-----------(1)----------------------------------------------------------->
  //                                   |
  //                                   |
  //                                   |
  //          +--------+           +---v----+             +--------+
  //<---(5)---+  ANR   +<---(5)----+ AEC    +<-----(4)----+  BF     <==(2,3)===
  //          +--------+           +--------+             +--------+

	do 
    {   // read audio and MIC data
        copy_audio (buf1);   
        copy_audio (buf2);
        copy_audio (buf3);

        // linear feedback of the loudspeaker to the MICs
        for (i=0; i<N/2; i++)
        {   buf2[i] = buf2[i] + buf1[i]; 
            buf3[i] = buf3[i] + buf1[i];
        }

        arm_beamformer_f32    (_NODE_RUN, (void *)&pt_BF_f32_instance, data_BF_f32, 0);
        xiph_libspeex_aec_f32 (_NODE_RUN, (void *)&pt_aec_f32_instance, data_aec_f32, 0);
        xiph_libspeex_anr_f32 (_NODE_RUN, (void *)&pt_anr_f32_instance, data_anr_f32, 0);

        // save the cleaned audio for ASR
        copy_audio (buf5);

    }   while (global_no_error);

#ifdef _MSC_VER 
    fclose(ptf_input_audio       );
    fclose(ptf_mic_L  );
    fclose(ptf_mic_R );
    fclose(ptf_to_asr            );
#endif 
}

/*--------------------------------------------------------------------------- */
