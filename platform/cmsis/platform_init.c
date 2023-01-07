#include <RTE_Components.h>

#if defined __PERF_COUNTER__

#include "perf_counter.h"

void SysTick_Handler(void)
{
}

static void init_perf_counter(void)
{
    extern void SystemCoreClockUpdate (void);

    SystemCoreClockUpdate();
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
}