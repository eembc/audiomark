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
 * Unless required by applicable law or agreed to in writing, link_software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
* Project:      DSP/ML stream-based computing
* Title:        link_public.h
* Description:  common constants and data types shared with SWC

* $Date:        May 2022
* $Revision:    V.0.0.1
*
* Target Processor: any
* -------------------------------------------------------------------- */

#ifndef __EE_AUDIOMARK_H
#define __EE_AUDIOMARK_H

#include <stdint.h>
#include <stdlib.h>
#include "ee_kws.h"
#include "ee_mfcc_f32.h"
#include "ee_nn_weights.h"

enum _component_req
{
    COMPONENT_BMF = 1,
    COMPONENT_AEC = 2,
    COMPONENT_ANR = 3,
    COMPONENT_KWS = 4,
};

#define AUDIO_CAPTURE_SAMPLING    16000
#define AUDIO_CAPTURE_FRAME_LEN_S 0.016
#define MONO                      1
#define SAMPLE_SIZE               2 // int16
/* #define AUDIO_NB_SAMPLES \
    (const int)(AUDIO_CAPTURE_SAMPLING * AUDIO_CAPTURE_FRAME_LEN_S) */
#define AUDIO_NB_SAMPLES 256
#define AUDIO_NB_BYTES   (AUDIO_NB_SAMPLES * SAMPLE_SIZE)

enum _command
{
    NODE_MEMREQ,        /* func(NODE_RESET, *instance, 0, 0) */
    NODE_RESET,         /* func(NODE_RESET, *instance, 0, 0) */
    NODE_RUN,           /* func(NODE_RUN, *instance, *in, *param) */
    NODE_SET_PARAMETER, /* func(NODE_SET_PARAMETER, *instance, index, *param) */
};

enum _memory_types
{
    DMEM      = 0,
    FAST_DMEM = 1,
    MEMBANK_TYPES,
};

#define PLATFORM_ARCH_64BIT

// assuming "int" is also the same size as "*int"
#ifdef PLATFORM_ARCH_32BIT
#define PTR_INT uint32_t
#endif
#ifdef PLATFORM_ARCH_64BIT
#define PTR_INT uint64_t
#endif

typedef struct
{
    PTR_INT p_data;
    PTR_INT size;
} xdais_buffer_t;

#define SETUP_XDAIS(SRC, DATA, SIZE) \
    {                                \
        SRC.p_data = (PTR_INT)DATA;  \
        SRC.size   = SIZE;           \
    }

// N.B. "REQ" currently comes from all_instances which is 32-bit values
#define LOCAL_ALLOC(PINST, REQ)               \
    {                                         \
        PINST = &(all_instances[idx_malloc]); \
        idx_malloc += 1 + REQ / 4;            \
    }

int32_t ee_abf_f32(int32_t, void **, void *, void *);
int32_t ee_aec_f32(int32_t, void **, void *, void *);
int32_t ee_anr_f32(int32_t, void **, void *, void *);
int32_t ee_kws_f32(int32_t, void **, void *, void *);

#endif
