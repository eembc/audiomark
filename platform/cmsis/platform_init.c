/* Header for target specific MPU definitions */
#ifndef CMSIS_device_header
/* CMSIS pack default header, containing the CMSIS_device_header definition */
#include "RTE_Components.h"
#endif
#include CMSIS_device_header
#include "cachel1_armv7.h"

#if defined __PERF_COUNTER__

#include "perf_counter.h"

void SysTick_Handler(void)
{
}

static void init_perf_counter(void)
{
    extern void SystemCoreClockUpdate (void);

#if (defined (IOTKit_CM33) || (IOTKit_CM33_FP) || (IOTKit_CM33_VHT) || (IOTKit_CM33_FP_VHT))
    /*  Correct SystemCoreClock to match the clock speed of the FPGA platform */
    SystemCoreClock = 20000000UL;
#else
    SystemCoreClockUpdate();
#endif
    init_cycle_counter(false);
}

#endif

__attribute__((constructor(255)))
void platform_init(void)
{
#if defined __PERF_COUNTER__
    init_perf_counter();
#endif

#ifdef RTE_Compiler_IO_STDOUT_User
   extern void stdout_init(void);
   stdout_init();
#endif

    SCB_EnableDCache();
    SCB_EnableICache();
}
