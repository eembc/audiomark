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


#define NFFT 128
#define LAGSTEP 16
#define FIXED_DIRECTION 0       /* fixed direction beamformer from [0.. LAGSTEP-1] <=> [-180deg .. +180deg] */
#define NFFTD2 (NFFT/2)
#define REAL 1
#define COMPLEX 2
#define MONO 1
#define STEREO 2

/* Fast coefficient structure */
typedef struct beamformer_f32_fastdata_static_t
{
    float old_left[NFFT*REAL], old_right[NFFT*REAL];
    uintPtr_t FFT128;
} beamformer_f32_fastdata_static_t;

/* "working" / scratch area */
typedef struct beamformer_f32_fastdata_working_t
{
    float X0[NFFT*COMPLEX], Y0[NFFT*COMPLEX], CY0[NFFTD2*COMPLEX];
    float XY[NFFTD2*COMPLEX], PHATNORM[NFFTD2*COMPLEX];
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


#endif