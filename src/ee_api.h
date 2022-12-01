/*
 * Copyright (C) EEMBC(R). All Rights Reserved
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#ifndef _EE_API_H
#define _EE_API_H

#include "ee_types.h"
#include "dsp/matrix_functions.h"
#include "dsp/transform_functions.h"
#include "dsp/statistics_functions.h"
#include "dsp/support_functions.h"

void *th_malloc(size_t size, int req);
void th_free(void * mem, int req);
void *th_memcpy(void *restrict dst, const void *restrict src, size_t n);
void *th_memset(void *b, int c, size_t len);
void *th_memmove(void *restrict dst, const void *restrict src, size_t n);

void th_softmax_i8(const int8_t *vec_in, const uint16_t dim_vec, int8_t *p_out);
void th_nn_init(void);
void th_nn_classify(const int8_t *p_input, int8_t *p_output);

/* This borrows _heavily_ from Arm CMSIS/DSP see their copyright. */

void th_int16_to_f32(const int16_t *p_src, ee_f32_t *p_dst, uint32_t len);

void th_f32_to_int16(const ee_f32_t *p_src, int16_t *p_dst, uint32_t len);

/* max(p_in) = *p_max = p_in[*p_index] */
void th_absmax_f32(const ee_f32_t *p_in,
                   uint32_t        len,
                   ee_f32_t       *p_max,
                   uint32_t       *p_index);

/* Each vector is a complex pair [r0, i0, r1, i0, ..., rn, in] */
/* C = A * B */
void th_cmplx_mult_cmplx_f32(const ee_f32_t *p_a,
                             const ee_f32_t *p_b,
                             ee_f32_t       *p_c,
                             uint32_t        len);

/* C = A + B */
void th_add_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len);

/* C = A - B */
void th_subtract_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len);

/* C = A * B */
void th_multiply_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len);

/* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */
void th_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

/* C = A + offset */
void th_offset_f32(ee_f32_t *p_a, ee_f32_t offset, ee_f32_t *p_c, uint32_t len);

/* C = log(A) */
void th_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

/* C[m] = A[m,n] * B[m] */
void th_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c);

/* C = A* */
void th_cmplx_conj_f32(const ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

/* R + iI = A dot B */
void th_cmplx_dot_prod_f32(const ee_f32_t *p_a,
                           const ee_f32_t *p_b,
                           uint32_t        len,
                           ee_f32_t       *p_r,
                           ee_f32_t       *p_i);

ee_status_t th_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length);

void th_rfft_f32(ee_rfft_f32_t *p_instance,
                 ee_f32_t      *p_in,
                 ee_f32_t      *p_out,
                 uint8_t        ifftFlag);

ee_status_t th_cfft_init_f32(ee_cfft_f32_t *p_instance, int fft_length);

void th_cfft_f32(ee_cfft_f32_t *p_instance,
                 ee_f32_t      *p_buf,
                 uint8_t        ifftFlag,
                 uint8_t        bitReverseFlag);

/* result = A dot B */
void th_dot_prod_f32(ee_f32_t *p_a,
                     ee_f32_t *p_b,
                     uint32_t  len,
                     ee_f32_t *p_result);

#endif