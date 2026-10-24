/**
 * Copyright 2026 Harshit Kumar Shivhare
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RVV_MDF_OPT_CONFIG_H
#define RVV_MDF_OPT_CONFIG_H

#if defined(__riscv_v_elen_fp) && __riscv_v_elen_fp >= 32

#define OVERRIDE_MDF_INNER_PROD
#define OVERRIDE_MDF_POWER_SPECTRUM
#define OVERRIDE_MDF_POWER_SPECTRUM_ACCUM
#define OVERRIDE_MDF_SPECTRAL_MUL_ACCUM
#define OVERRIDE_MDF_SPECTRAL_MUL_ACCUM16
#define OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ
#define OVERRIDE_MDF_ADJUST_PROP
#define OVERRIDE_MDF_PREEMPH_FLT
#define OVERRIDE_MDF_STRIDED_PREEMPH_FLT
#define OVERRIDE_MDF_VEC_SUB
#define OVERRIDE_MDF_VEC_SUB_INT16
#define OVERRIDE_MDF_VEC_ADD
#define OVERRIDE_MDF_VEC_MULT
#define OVERRIDE_MDF_VEC_SCALE
#define OVERRIDE_MDF_SMOOTHED_ADD
#define OVERRIDE_MDF_SMOOTH_FE_NRG
#define OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR
#define OVERRIDE_MDF_NORM_LEARN_RATE_CALC
#define OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC

#endif

#endif /* RVV_MDF_OPT_CONFIG_H */
