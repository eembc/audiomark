/*
 * Copyright (C) EEMBC(R). All Rights Reserved
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#ifndef __EE_MFCC_F32_H
#define __EE_MFCC_F32_H

#include <math.h>
#include "ee_mfcc_f32_tables.h"
#include "ee_api.h"
#include "ee_nn_weights.h"

/* 40ms frame of 16 kHz audio is 640 16-bit samples. */
#define FRAME_LEN 640
/* Pre-defined */
#define NUM_MFCC_FEATURES 10
#define MFCC_DEC_BITS     1
// frame_len_padded = pow(2,ceil((log(FRAME_LEN)/log(2)))); == 1024
#define PADDED_FRAME_LEN 1024
#define FFT_LEN          PADDED_FRAME_LEN

// NUM_FRAMES is in ee_nn_weights.h
#define MFCC_FIFO_BYTES (NUM_MFCC_FEATURES * NUM_FRAMES)

typedef struct mfcc_instance_t
{
    ee_f32_t      mfcc_input_frame[FFT_LEN];
    ee_f32_t      mfcc_out[NUM_MFCC_FEATURES];
    ee_f32_t      tmp[FFT_LEN + 2];
    ee_rfft_f32_t rfft_instance;
} mfcc_instance_t;

ee_status_t ee_mfcc_f32_init(mfcc_instance_t *);
void        ee_mfcc_f32_compute(mfcc_instance_t *, const int16_t *, int8_t *);

#endif /* __EE_MFCC_H */
