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
#include <stdbool.h>
#include "ee_audiomark.h"

#define TEST_NBUFFERS 104U
#define NSAMPLES      256U
#define NFRAMEBYTES   512U

#define SNRM50DB 0.003162f

extern const int16_t p_input[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_expected[TEST_NBUFFERS][NSAMPLES];

static int16_t p_input_sub[NSAMPLES];

static xdais_buffer_t xdais[1];

// Used deep inside speex_alloc; assigned by component NODE_RESET
char *spxGlobalHeapPtr;
char *spxGlobalHeapEnd;
long  cumulatedMalloc;

int
main(int argc, char *argv[])
{
    bool      err           = false;
    uint32_t  memreq        = 0;
    uint32_t *p_req         = &memreq;
    void     *inst          = NULL;
    uint32_t  parameters[1] = { 0 };
    uint32_t  A             = 0;
    uint32_t  B             = 0;
    float     ratio         = 0.0f;

    if (ee_anr_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL))
    {
        printf("ANR NODE_MEMREQ failed\n");
        return -1;
    }
    printf("ANR MEMREQ = %d bytes\n", memreq);

    inst = malloc(memreq);
    if (!inst)
    {
        printf("ANR malloc() fail\n");
        return -1;
    }

    // ANR uses an in-place buffer
    SETUP_XDAIS(xdais[0], p_input_sub, NFRAMEBYTES);

    // SpeeX will call speex_malloc() on NODE_RESET giving us the real mem used
    cumulatedMalloc = 0;
    if (ee_anr_f32(NODE_RESET, (void **)&inst, xdais, &parameters))
    {
        printf("ANR NODE_RESET failed\n");
        return -1;
    }

    // Sanity check the actual allocation from Speex
    printf("ANR SpeeX cumulatedMalloc = %ld bytes\n", cumulatedMalloc);
    if (cumulatedMalloc > memreq)
    {
        printf("ANR ran out of memory but didn't complain!\n");
        return -1;
    }

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(p_input_sub, &p_input[i], NFRAMEBYTES);

        if (ee_anr_f32(NODE_RUN, (void **)&inst, xdais, NULL))
        {
            err = true;
            printf("ANR NODE_RUN failed\n");
            break;
        }

        A = 0;
        B = 0;

        for (unsigned j = 0; j < NSAMPLES; ++j)
        {
            A += abs(p_input_sub[j]);
            B += abs(p_input_sub[j] - p_expected[i][j]);

#ifdef DEBUG_EXACT_BITS
            if (p_input_sub[j] != p_expected[i][j])
            {
                err = true;
                printf("S[%03d]B[%03d]O[%-5d]E[%-5d] ... FAIL\n",
                       i,
                       j,
                       p_input_sub[j],
                       p_expected[i][j]);
            }
#endif
        }

        ratio = (float)B / (float)A;
        if (ratio > SNRM50DB)
        {
            err = true;
            printf("ANR FAIL: Frame #%d exceeded -50 dB SNR\n", i);
        }
    }

    if (err)
    {
        printf("ANR test failed\n");
        return -1;
    }
    printf("ANR test passed\n");
    return 0;
}
