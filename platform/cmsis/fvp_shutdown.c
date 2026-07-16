/*
 * Copyright (C) 2026 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(FVP_SHUTDOWN_ON_EXIT)

extern int stdout_putchar(int ch);
extern int platform_main(void);

#define ASCII_EOT (0x04)

static void fvp_send_eot(void)
{
    (void)stdout_putchar(ASCII_EOT);
}

/*
 * Some bare-metal startup code calls main() and then loops forever instead of
 * calling exit(). For FVP regression runs, the solution defines
 * main=platform_main for the application sources and this wrapper becomes the
 * real entry point. This keeps AudioMark core and test sources unchanged while
 * still giving the UART model an EOT character to terminate on.
 */
#if defined(main)
#undef main
#endif

int main(void)
{
    int status = platform_main();
    fvp_send_eot();
    return status;
}

__attribute__((destructor(255))) static void fvp_shutdown_on_exit(void)
{
    fvp_send_eot();
}

#if defined(__ARMCC_VERSION)
void _sys_exit(int status)
{
    (void)status;
    fvp_send_eot();
    for (;;) {
    }
}
#else
__attribute__((noreturn)) void _exit(int status)
{
    (void)status;
    fvp_send_eot();
    for (;;) {
    }
}
#endif

#endif
