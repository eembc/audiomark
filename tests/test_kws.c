/**
 * Copyright (C) 2025 SPEC Embedded Group
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
#include "ee_types.h"
#include "ee_audiomark.h"

#define NBUFFERS 93
#define NINFERS  73
#define NSAMPLES 256
#define NCLASSES 12


//#define DEBUG_EXACT_BITS
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

extern const int16_t p_input[NBUFFERS][NSAMPLES];
extern const int8_t p_expected[NINFERS][NCLASSES];

// Used deep inside audiomark core
char     *spxGlobalHeapPtr;
char     *spxGlobalHeapEnd;

int32_t ee_kws_f32(int32_t command,
                   void  **pp_instance,
                   void   *p_data,
                   void   *p_params);

static int16_t aec_output[256]; // 5
static int16_t audio_fifo[13 * 64]; // 6
static int8_t mfcc_fifo[490];   // 7
static int8_t classes[12];      // 8
static xdais_buffer_t xdais[4];

static const float32_t ROW_JSD_THRESH = 0.015f;     // per-row JSD tolerance (0..1)
static const float32_t MEAN_JSD_THRESH = 0.0025f;   // mean across all rows
static const float32_t MAX_JSD_THRESH = 0.05f;      // max tolerable JSD
static const float32_t MAX_TOL_JSD_RATIO = 0.01f;   // at max 1% of frames rows above ROW_JSD_THRESH

// Compute Jensen-Shannon divergence between two distributions P and Q (length NCLASSES).
// Inputs are probabilities that sum to 1. Returns value in [0, 1].
static float32_t ee_kws_ut_jensenshannon_divergence_f32(const float32_t *P, const float32_t *Q)
{
    static const float32_t EPS = 1e-12f;
    float32_t jsd = 0.0f;
    for (int i = 0; i < NCLASSES; ++i)
    {
        float32_t p = P[i] <= 0.0f ? EPS : P[i];
        float32_t q = Q[i] <= 0.0f ? EPS : Q[i];

        float32_t m = 0.5f * (p + q);
        jsd += 0.5f * p * (log2f(p) - log2f(m));
        jsd += 0.5f * q * (log2f(q) - log2f(m));
    }
    return jsd;
}

// Normalize one row of int8 probabilities to float32_ts that sum to 1.
static void ee_kws_ut_normalize_q8_proba_f32(const int8_t *row, float32_t *out_prob)
{
    int       sum = 0;
    for (int i = 0; i < NCLASSES; ++i)
    {

        sum += (float32_t) (row[i] + 128);
    }

    float32_t inv = 1.0f / (float32_t) sum;
    for (int i = 0; i < NCLASSES; ++i)
        out_prob[i] = (float32_t) (row[i] + 128) * inv;
}

int
main(int argc, char *argv[])
{
    int       err = 0;
    int       new_inference = 0;
    const int8_t *p_check = NULL;
    int       idx_check = 0;
    uint32_t  memreq = 0;
    uint32_t *p_req = &memreq;
    void     *memory = NULL;
    void     *inst = NULL;
    uint32_t  A = 0;
    uint32_t  B = 0;
    int       i, j;
    int       inferences = 0;
    float32_t mean_jsd = 0.0f;
    uint32_t  jsd_violation_cnt = 0;
    float32_t max_jsd = 0.0f;

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

    for (i = 0; i < NBUFFERS; ++i)
    {
        memcpy(aec_output, p_input[i], 512 /* 256 samples @ 2bytes@ */);
        ee_kws_f32(NODE_RUN, (void **)&inst, xdais, &new_inference);

        /* printf("inferences=%d, i=%d, idx_check=%d\n", inferences, i, idx_check); */

        /* check both classes are noises */
        A = B = -127;
        p_check = p_expected[idx_check];
        for (j = 0; j < NCLASSES; ++j)
        {  A = MAX(A, classes[j]); /* Look for max value in the calculated result */
               B = MAX(B, p_check[j]); /* Look for max value in the expected result */
            }
        if ( (A < 0)  && (B < 0)) {
          if (new_inference) {
            ++inferences;
            ++idx_check;
          }
          continue; /* Both are less than 0, considered as noise and skip */
        }

        if (new_inference)
        {
            float32_t ref_proba[NCLASSES], classes_proba[NCLASSES];

            ++inferences;
            p_check = p_expected[idx_check];

            /* compare probabilities distribution using JSD */
            ee_kws_ut_normalize_q8_proba_f32(p_check, ref_proba);
            ee_kws_ut_normalize_q8_proba_f32(classes, classes_proba);
            float32_t jsd = ee_kws_ut_jensenshannon_divergence_f32(ref_proba, classes_proba);

            mean_jsd += jsd;
            if (jsd > max_jsd)
                max_jsd = jsd;

            if (jsd > ROW_JSD_THRESH)
                jsd_violation_cnt++;

            ++idx_check;

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
        printf("KWS expected %d inferences but got %d\n", NINFERS, inferences);
    }

    /*
     * Jensen Shannnon Divergences checks
     */
    mean_jsd /= (float32_t) inferences;
    float32_t jsd_violation_ratio = (float32_t) jsd_violation_cnt / (float32_t) inferences;

    if ((max_jsd > MAX_JSD_THRESH) || (jsd_violation_ratio > MAX_TOL_JSD_RATIO))
    {
        err = 1;
        printf("KWS / JSD violations: %u of %d rows (%.2f%%), max=%.5f, mean=%.5f\n",
            jsd_violation_cnt, inferences, 100.0f * jsd_violation_ratio, max_jsd, mean_jsd);
    }

    if (mean_jsd > MEAN_JSD_THRESH)
    {
        err = 1;
        printf("KWS / mean error beyond limit (mean JSD = %f)\n", mean_jsd);
    }

    if (err)
    {
        printf("KWS test failed\n");
        return -1;
    }

    printf("KWS test passed\n");
    return 0;
}
