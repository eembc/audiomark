/**
 * Copyright (C) 2022 EEMBC
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

#include <stddef.h>
#include <string.h>
#include <riscv_dsp_types.h>

#define TH_FLOAT32_TYPE               float
#define TH_RFFT_INSTANCE_FLOAT32_TYPE riscv_rfft_fast_instance
#define TH_CFFT_INSTANCE_FLOAT32_TYPE riscv_cfft_instance

/*
   struct for matrix type is not defined yet because audiomark
   expects a particular nomenclature for the MFCC functions and
   ee_matrix_f32_t is already defined according to that in
   src/ee_types.h. the ARM port defines custom structs for this
   and manages the conversions manually.
*/

#endif /* __TH_TYPES_H */
