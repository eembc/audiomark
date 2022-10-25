/*
 * Copyright (C) 2022 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, link_software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
* Project:      DSP/ML stream-based computing
* Title:        link_public.h
* Description:  common constants and data types shared with SWC

* $Date:        May 2022
* $Revision:    V.0.0.1
*
* Target Processor: any
* -------------------------------------------------------------------- */


#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#include <stdint.h>
#include <string.h>

typedef void *swc_instance;
typedef uint8_t *uintPtr_t;

enum _command 
{ 
    NODE_MEMREQ,            /* func(NODE_RESET, *instance, 0, 0) */
    NODE_RESET,             /* func(NODE_RESET, *instance, 0, 0) */
    NODE_RUN,               /* func(NODE_RUN, *instance, *in, *param) */
    NODE_SET_PARAMETER,     /* func(NODE_SET_PARAMETER, *instance, index, *param) */
};

enum _memory_types
{   
    DMEM = 0, 
    FAST_DMEM = 1,  
    MEMBANK_TYPES,  
};

#define PLATFORM_ARCH_64BIT

// assuming "int" is also the same size as "*int"
#ifdef PLATFORM_ARCH_32BIT 
#define PTR_INT uint32_t
#endif
#ifdef PLATFORM_ARCH_64BIT 
#define PTR_INT uint64_t
#endif

typedef struct {
  PTR_INT data_struct[2];
} data_buffer_t;



#endif /* #ifndef __link_PUBLIC_H__ */
/*
 * -----------------------------------------------------------------------
 */
