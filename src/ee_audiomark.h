/**
 * Copyright (C) 2022 EEMBC
 * Copyright (C) 2022 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#ifndef __EE_AUDIOMARK_H
#define __EE_AUDIOMARK_H

#include <stdint.h>
#include <stdlib.h>
#include "ee_kws.h"
#include "ee_mfcc_f32.h"
#include "ee_data/ee_data.h"

enum _component_req
{
    COMPONENT_BMF = 1,
    COMPONENT_AEC = 2,
    COMPONENT_ANR = 3,
    COMPONENT_KWS = 4,
};

#define SAMPLING_FREQ_HZ 16000
#define FRAME_LEN_SEC    0.016
#define MONO             1
#define BYTES_PER_SAMPLE 2 // int16
/* #define SAMPLES_PER_AUDIO_FRAME \
    (const int)(SAMPLING_FREQ_HZ * FRAME_LEN_SEC) */
#define SAMPLES_PER_AUDIO_FRAME 256
#define BYTES_PER_AUDIO_FRAME   (SAMPLES_PER_AUDIO_FRAME * BYTES_PER_SAMPLE)

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

// These are from the component files
int32_t ee_abf_f32(int32_t, void **, void *, void *);
int32_t ee_aec_f32(int32_t, void **, void *, void *);
int32_t ee_anr_f32(int32_t, void **, void *, void *);
int32_t ee_kws_f32(int32_t, void **, void *, void *);

int  ee_audiomark_initialize(void);
int  ee_audiomark_run(void);
void ee_audiomark_release(void);

#endif
