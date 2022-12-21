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

#ifndef __TH_TYPES_H
#define __TH_TYPES_H

#include "dsp/matrix_functions.h"
#include "dsp/transform_functions.h"
#include "dsp/statistics_functions.h"
#include "dsp/support_functions.h"

#define TH_FLOAT32_TYPE                 float
#define TH_MATRIX_INSTANCE_FLOAT32_TYPE arm_matrix_instance_f32
#define TH_RFFT_INSTANCE_FLOAT32_TYPE   arm_rfft_fast_instance_f32
#define TH_CFFT_INSTANCE_FLOAT32_TYPE   arm_cfft_instance_f32

#endif /* __TH_TYPES_H */
