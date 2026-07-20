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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ee_audiomark.h"

// There are several POSIX assumptions in this implementation.
#if defined __linux__ || __APPLE__
#include <time.h>
#elif defined _WIN32
#include <sys\timeb.h>
#elif defined __arm__
#include <RTE_Components.h>
#if defined __PERF_COUNTER__
#include "perf_counter.h"
#endif
#else
#error "Operating system not recognized"
#endif
#include <assert.h>
#ifndef AUDIOMARK_LOG_INFO
#define AUDIOMARK_LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef AUDIOMARK_LOG_ERROR
#define AUDIOMARK_LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif



uint64_t
th_microseconds(void)
{
    uint64_t usec = 0;
#if defined __linux__ || __APPLE__
    const long      NSEC_PER_SEC      = 1000 * 1000 * 1000;
    const long      TIMER_RES_DIVIDER = 1000;
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    usec = t.tv_sec * (NSEC_PER_SEC / TIMER_RES_DIVIDER)
           + t.tv_nsec / TIMER_RES_DIVIDER;
#elif defined _WIN32
    struct timeb t;
    ftime(&t);
    usec = ((uint64_t)t.time) * 1000 * 1000 + ((uint64_t)t.millitm) * 1000;
#elif defined __arm__ && defined __PERF_COUNTER__
    usec = (uint64_t)get_system_us();
#else
#error "Operating system not recognized"
#endif
    return usec;
}

bool
time_audiomark_run(uint32_t iterations, uint64_t *dt)
{
    uint64_t t0  = 0;
    uint64_t t1  = 0;
    bool     err = false;

    t0 = th_microseconds();
    for (uint32_t i = 0; i < iterations; ++i)
    {
        if (ee_audiomark_run())
        {
            err = true;
            break;
        }
    }
    t1  = th_microseconds();
    *dt = t1 - t0;
    return err;
}

int
main(void)
{
    bool     err        = false;
    uint32_t iterations = 1;
    uint64_t dt         = 0;

    AUDIOMARK_LOG_INFO("Initializing");

    if (ee_audiomark_initialize())
    {
        AUDIOMARK_LOG_ERROR("Failed to initialize");
        return -1;
    }

    AUDIOMARK_LOG_INFO("Computing run speed");

    do
    {
        iterations *= 2;
        err = time_audiomark_run(iterations, &dt);
        if (err)
        {
            break;
        }
    } while (dt < 1e6f);

    if (err)
    {
        AUDIOMARK_LOG_ERROR("Failed to compute iteration speed");
        goto exit;
    }

    // Must run for 10 sec. or at least 10 iterations
    float scale = 11e6f / dt;
    iterations  = (uint32_t)((float)iterations * scale);
    iterations  = iterations < 10 ? 10 : iterations;

    AUDIOMARK_LOG_INFO("Measuring");

    err = time_audiomark_run(iterations, &dt);
    if (err)
    {
        AUDIOMARK_LOG_ERROR("Failed main performance run");
        goto exit;
    }

    /**
     * The input stream is 24e3 samples at 16 kHz, which means to exactly
     * match the throughput of the stream the score would be one iteration
     * per 1.5 seconds. The score is how many times faster than the ADC
     * the pipeline runs. x 1000 to make it a bigger number.
     */
    float sec   = (float)dt / 1.0e6f;
    float score = (float)iterations / sec * 1000.f * (1.0f / 1.5f);

    AUDIOMARK_LOG_INFO("Total runtime    : %.3f seconds", sec);
    AUDIOMARK_LOG_INFO("Total iterations : %d iterations", iterations);
    AUDIOMARK_LOG_INFO("Score            : %f AudioMarks", score);
exit:
    ee_audiomark_release();
    return err ? -1 : 0;
}
