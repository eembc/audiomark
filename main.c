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

int  audiomark_initialize(void);
int  audiomark_run(void);
void audiomark_release(void);

// There are several POSIX assumptions in this implementation.
#if defined __linux__ || __APPLE__
#include <time.h>
#elif defined _WIN32
#include <sys\timeb.h>
#elif defined __ARM_PERF_COUNTER__
extern void SystemCoreClockUpdate (void);
#include "perf_counter.h"
static int is_perf_counter_init = false;
#else
#error "Operating system not recognized"
#endif
#include <assert.h>

#if defined __ARM_PERF_COUNTER__
extern uint32_t SystemCoreClock;

void SysTick_Handler(void)
{
}

void init_perf_counter(void)
{
    SystemCoreClockUpdate();
    init_cycle_counter(false);
}
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
#elif defined __ARM_PERF_COUNTER__
    if(!is_perf_counter_init) {
        init_perf_counter();
        is_perf_counter_init = true;
    }
    //printf("xxx %lld %d SystemCoreClock  %d\n", get_system_ticks(), get_system_us(), SystemCoreClock );
    usec = get_system_ticks();
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
        if (audiomark_run())
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

    printf("Initializing\n");

    if (audiomark_initialize())
    {
        printf("Failed to initialize\n");
        return -1;
    }

    printf("Computing run speed\n");

    do
    {
        iterations *= 2;
        err = time_audiomark_run(iterations, &dt);
        if (err)
        {
            break;
        }
    } while (dt < 1e6);

    if (err)
    {
        printf("Failed to compute iteration speed\n");
        goto exit;
    }

    // Must run for 10 sec. or at least 10 iterations
    float scale = 11e6 / dt;
    iterations  = (uint32_t)((float)iterations * scale);
    iterations  = iterations < 10 ? 10 : iterations;

    printf("Measuring\n");

    err = time_audiomark_run(iterations, &dt);
    if (err)
    {
        printf("Failed main performance run\n");
        goto exit;
    }

    /**
     * The input stream is 24e3 samples at 16 kHz, which means to exactly
     * match the throughput of the stream the score would be one iteration
     * per 1.5 seconds. The score is how many times faster than the ADC
     * the pipeline runs. x 1000 to make it a bigger number.
     */
    float sec   = (float)dt / 1.0e6f;
    float score = (float)iterations / sec * 1000.f * (1 / 1.5f);

    printf("Total runtime    : %.3f seconds\n", sec);
    printf("Total iterations : %d iterations\n", iterations);
    printf("Score            : %f AudioMarks\n", score);
exit:
    audiomark_release();
    return err ? -1 : 0;
}
