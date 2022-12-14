/**
 * Copyright (C) 2022 EEMBC
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#include <stdlib.h>
#include <stdio.h>
#include "ee_mfcc_f32.h"

extern const int16_t p_input[FRAME_LEN];
extern const int8_t  p_expected[NUM_MFCC_FEATURES];

int8_t p_output[NUM_MFCC_FEATURES];

mfcc_instance_t *mfcc_instance;

int
main(int argc, char *argv[])
{
    int err = 0;

    mfcc_instance = malloc(sizeof(mfcc_instance_t));
    ee_mfcc_f32_init(mfcc_instance);
    ee_mfcc_f32_compute(mfcc_instance, p_input, p_output);

    for (int i = 0; i < NUM_MFCC_FEATURES; ++i)
    {
        if (p_output[i] != p_expected[i])
        {
            err = 1;
            printf("feature[%d]: Computed %d, expected %d ... FAIL\n",
                   i,
                   p_output[i],
                   p_expected[i]);
        }
    }

    if (err)
    {
        printf("MFCC test failed\n");
        return -1;
    }
    printf("MFCC test passed\n");
    return 0;
}