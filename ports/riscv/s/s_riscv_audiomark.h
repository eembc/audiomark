/**
 * Copyright (C) 2024 SPEC Embedded Group
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

#ifndef RISCV_AUDIOMARK_H
#define RISCV_AUDIOMARK_H

#include "ee_audiomark.h"
#include "ee_api.h"

ee_status_t s_riscv_cfft_init_f32(ee_cfft_f32_t *p_instance, int fft_length);

void s_riscv_cfft_f32(ee_cfft_f32_t *p_instance,
                      ee_f32_t      *p_buf,
                      uint8_t        ifftFlag,
                      uint8_t        bitReverseFlagR);

ee_status_t s_riscv_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length);

void s_riscv_rfft_f32(ee_rfft_f32_t *p_instance,
                      ee_f32_t      *p_in,
                      ee_f32_t      *p_out,
                      uint8_t        ifftFlag);

void s_riscv_absmax_f32(const ee_f32_t *p_in,
                        uint32_t        len,
                        ee_f32_t       *p_max,
                        uint32_t       *p_index);

void s_riscv_cmplx_mult_cmplx_f32(const ee_f32_t *p_a,
                                  const ee_f32_t *p_b,
                                  ee_f32_t       *p_c,
                                  uint32_t        len);

void s_riscv_cmplx_conj_f32(const ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

void s_riscv_cmplx_dot_prod_f32(const ee_f32_t *p_a,
                                const ee_f32_t *p_b,
                                uint32_t        len,
                                ee_f32_t       *p_r,
                                ee_f32_t       *p_i);

void s_riscv_int16_to_f32(const int16_t *p_src, ee_f32_t *p_dst, uint32_t len);

void s_riscv_f32_to_int16(const ee_f32_t *p_src, int16_t *p_dst, uint32_t len);

void s_riscv_add_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len);

void s_riscv_subtract_f32(ee_f32_t *p_a,
                          ee_f32_t *p_b,
                          ee_f32_t *p_c,
                          uint32_t  len);

void s_riscv_dot_prod_f32(ee_f32_t *p_a,
                          ee_f32_t *p_b,
                          uint32_t  len,
                          ee_f32_t *p_result);

void s_riscv_multiply_f32(ee_f32_t *p_a,
                          ee_f32_t *p_b,
                          ee_f32_t *p_c,
                          uint32_t  len);

void s_riscv_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

void s_riscv_offset_f32(ee_f32_t *p_a,
                        ee_f32_t  offset,
                        ee_f32_t *p_c,
                        uint32_t  len);

void s_riscv_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

void s_riscv_mat_vec_mult_f32(ee_matrix_f32_t *p_a,
                              ee_f32_t        *p_b,
                              ee_f32_t        *p_c);

void s_riscv_nn_init(void);

ee_status_t s_riscv_nn_classify(void);

#endif
