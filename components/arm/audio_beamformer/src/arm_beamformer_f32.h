/*
 * Copyright (C) 2022 Arm Limited or its affiliates. All rights reserved.
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
 */

/* ----------------------------------------------------------------------
* Project:      link & audioMark
* Title:        arm_beamformer.h
* Description:  coefficients used for signal analysis and synthesis
*
* $Date:        May 26h 2022
* $Revision:    V.0.0.1
*
* Target Processor: all
* -------------------------------------------------------------------- */

#ifndef __ARM_BEAMFORMER_H__
#define __ARM_BEAMFORMER_H__

#include "dsp/transform_functions.h"

//#define ARM_TABLE_TWIDDLECOEF_F32_128
//#define ARM_TABLE_BITREVIDX_FLT_128

#define NFFT 128
#define NFFTD2 (NFFT/2)
#define LAGSTEP 16
#define FIXED_DIRECTION 0       /* fixed direction beamformer from [0.. LAGSTEP-1] <=> [-180deg .. +180deg] */
#define REAL 1
#define COMPLEX 2
#define MONO 1
#define STEREO 2
#define SAMP_SIZE 4             /* size of float */
#define LEN_BM_ADF 6

typedef struct adapBF_f32_fastdata_prms_t
{
    float32_t alpha_BM_NLMS;
    float32_t DS_DET_TH;
    float32_t ep_GSC;
} adapBF_f32_fastdata_prms_t;

typedef struct adapBF_f32_fastdata_static_t
{
    float32_t states_BM_ADF[NFFT/2 + 1][LEN_BM_ADF*2];
    float32_t coefs_BM_ADF[NFFT/2 + 1][LEN_BM_ADF*2];
    float32_t Norm_out_BM[NFFT/2 + 1];
    float32_t lookBF_out[NFFT/2 + 1];
    float32_t GSC_det_avg;
    uint8_t adptBF_coefs_update_enable;
} adapBF_f32_fastdata_static_t;


/* Fast coefficient structure */
typedef struct beamformer_f32_fastdata_static_t
{
    float old_left[NFFTD2*COMPLEX], old_right[NFFTD2*COMPLEX];
    float ola_new[NFFTD2];
    arm_rfft_fast_instance_f32 rS;
    arm_cfft_instance_f32 cS;
} beamformer_f32_fastdata_static_t;

/* "working" / scratch area */
typedef struct beamformer_f32_fastdata_working_t
{
    float mic[NFFT*COMPLEX+2];
    float BM[NFFT*COMPLEX+2], BF[NFFT*COMPLEX+2]; 
    float X0[NFFT*COMPLEX+2], Y0[NFFT*COMPLEX+2]; 
    float CY0[NFFTD2*COMPLEX+2];
    float XY[NFFTD2*COMPLEX];
    float PHATNORM[NFFTD2*COMPLEX];
    float allDerot[LAGSTEP];
    float corr; uint32_t icorr;
} beamformer_f32_fastdata_working_t;


typedef struct
{
    float *wrot;
    float *window;
    beamformer_f32_fastdata_static_t *st;      /* fast static memory area */
    beamformer_f32_fastdata_working_t *w;      /* fast working memory area */
} beamformer_f32_instance;

extern adapBF_f32_fastdata_prms_t bf_prms;
extern  adapBF_f32_fastdata_static_t bf_mem;
void adapBF_init(adapBF_f32_fastdata_prms_t* bf_prms, adapBF_f32_fastdata_static_t* bf_mem);
void arm_beamformer_f32_run(beamformer_f32_instance *instance, 
                        int16_t* input_buffer_left, int16_t* input_buffer_right, int32_t input_buffer_size, 
                        int16_t* output_buffer, int32_t *input_samples_consumed, int32_t *output_samples_produced, 
                        int32_t* returned_state);
#endif
