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
#include "ee_types.h"
#include "ee_audiomark.h"

#define NBUFFERS 93
#define NINFERS  73
#define NSAMPLES 256
#define NCLASSES 12

extern const int16_t p_input[NBUFFERS][NSAMPLES];
extern const int8_t  p_expected[NINFERS][NCLASSES];

int32_t ee_kws_f32(int32_t command,
                   void  **pp_instance,
                   void   *p_data,
                   void   *p_params);

static int16_t        aec_output[256];     // 5
static int16_t        audio_fifo[13 * 64]; // 6
static int8_t         mfcc_fifo[490];      // 7
static int8_t         classes[12];         // 8
static xdais_buffer_t xdais[4];

int
main(int argc, char *argv[])
{
    int           err           = 0;
    int           new_inference = 0;
    const int8_t *p_check       = NULL;
    int           idx_check     = 0;
    uint32_t      memreq        = 0;
    uint32_t     *p_req         = &memreq;
    void         *memory        = NULL;
    void         *inst          = NULL;

    int inferences = 0;

    ee_kws_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);

    printf("KWS F32 MEMREQ = %d bytes\n", memreq);
    memory = malloc(memreq);
    if (!memory)
    {
        printf("malloc() fail\n");
        return -1;
    }
    inst = (void *)memory;
    SETUP_XDAIS(xdais[0], aec_output, 512);
    SETUP_XDAIS(xdais[1], audio_fifo, 13 * 64 * 2);
    SETUP_XDAIS(xdais[2], mfcc_fifo, 490);
    SETUP_XDAIS(xdais[3], classes, 12);

    ee_kws_f32(NODE_RESET, (void **)&inst, NULL, NULL);

    for (int i = 0; i < NBUFFERS; ++i)
    {
        memcpy(aec_output, p_input[i], 512 /* 256 samples @ 2bytes@ */);
        ee_kws_f32(NODE_RUN, (void **)&inst, xdais, &new_inference);

        if (new_inference)
        {
            ++inferences;
            p_check = p_expected[idx_check];
            ++idx_check;

            for (int j = 0; j < NCLASSES; ++j)
            {
                if (classes[j] != p_check[j])
                {
                    err = 1;
                    printf("buffer[%d]class[%d]: Got %d, expected %d - FAIL\n",
                           i,
                           j,
                           classes[j],
                           p_check[j]);
                }
            }
        }
    }

    if (inferences == 0)
    {
        err = 1;
        printf("KWS did not perform any inferences\n");
    }

    if (inferences != NINFERS)
    {
        err = 1;
        printf("KWS expected %d inferences but got %d\n", inferences, NINFERS);
    }

    if (err)
    {
        printf("KWS test failed\n");
        return -1;
    }

    printf("KWS test passed\n");
    return 0;
}
