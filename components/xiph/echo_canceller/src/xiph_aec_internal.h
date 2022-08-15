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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      Arm C300 Voice Demo Front end
 * Title:        swc_src_internal.h
 * Description:  software component for rate conversion

 * $Date:        May 2022
 * $Revision:    V.0.0.1
 *
 * Target Processor:  Cortex-M cores
 * -------------------------------------------------------------------- */

//#include <stdint.h>
//#define FIXED_POINT 1
//#include "../../../speexdsp-master/include/speex/speex_preprocess.h"
//#include "../../../speexdsp-master/libspeexdsp/arch.h"

//const int8_t swc_header [] = {
//    00, 01, /* LINK V1 */
//    00, 00, /* source code delivery */ 
//    83, 87, 67, 32, 83, 80, 69, 69, 88, 32, 83, 82, 67, 00,  /* "SWC SPEEX SRC" */
//};
//
//const int8_t swc_activation_flag [] = {
//    00, 00, 00, 01,
//};
//
//typedef struct demoState {
//
//    /* noise supressor context */
//    SpeexPreprocessState *preprocessSt;
//    spx_word16_t   *ref;
//
//    int             noiseRedFrameSz;
//    int             noiseRedAttenuationLvl;
//
//} swcSrcState;