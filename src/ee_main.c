/*---------------------------------------------------------------------------
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
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
 *---------------------------------------------------------------------------*/

#include <stdio.h>

int  audiomark_initialize(void);
int  audiomark_run(void);
void audiomark_release(void);

int
main(void)
{
    if (audiomark_initialize())
    {
        printf("Failed to initialize\n");
        return -1;
    }
    if (audiomark_run())
    {
        printf("Run failed\n");
        return -1;
    }

    audiomark_release();

    return 0;
}
