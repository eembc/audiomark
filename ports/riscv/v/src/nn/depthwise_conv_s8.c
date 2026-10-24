/*
 * Copyright 2026 Harshit Kumar Shivhare
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
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

#include "rvv_support_guard.h"

#include "functions.h"
#include "rvv_support_functions.h"

#include <stdint.h>
#include <stdbool.h>

#define KERNEL_DIM     3
#define INPUT_W        5
#define INPUT_H        25
#define INPUT_CH       64
#define PAD_W          1
#define PAD_H          1
#define STRIDE_W       1
#define STRIDE_H       1
#define OUTPUT_W       5
#define OUTPUT_H       25
#define INPUT_OFFSET   128
#define OUTPUT_OFFSET  (-128)
#define OUT_ACT_MIN    (-128)
#define OUT_ACT_MAX    127
#define COL_STRIDE     INPUT_CH
#define INP_ROW_STRIDE (INPUT_CH * INPUT_W)
#define KER_ROW_STRIDE (INPUT_CH * KERNEL_DIM)
#define INPUT_X_STEP   (STRIDE_W * COL_STRIDE)

static inline vint32m4_t
rvv_acc_tap(vint32m4_t acc, const int8_t *ip, const int8_t *kp, size_t vl)
{
    vint8m1_t  in8  = __riscv_vle8_v_i8m1(ip, vl);
    vint8m1_t  k8   = __riscv_vle8_v_i8m1(kp, vl);
    vint16m2_t in16 = __riscv_vsext_vf2_i16m2(in8, vl);
    vint16m2_t k16  = __riscv_vsext_vf2_i16m2(k8, vl);

    in16 = __riscv_vadd_vx_i16m2(in16, (int16_t)INPUT_OFFSET, vl);

    return __riscv_vwmacc_vv_i32m4(acc, in16, k16, vl);
}

static inline vint32m4_t
rvv_requantize(vint32m4_t acc,
               vint32m4_t v_mult,
               vint32m4_t v_ls,
               vint32m4_t v_rs,
               size_t     vl)
{
    vint32m4_t pre = __riscv_vsll_vv_i32m4(
        acc, __riscv_vreinterpret_v_i32m4_u32m4(v_ls), vl);
    vint32m4_t high = __riscv_vsmul_vv_i32m4(pre, v_mult, __RISCV_VXRM_RNU, vl);

    vint32m4_t rmask = __riscv_vnot_v_i32m4(
        __riscv_vsll_vv_i32m4(__riscv_vmv_v_x_i32m4(-1, vl),
                              __riscv_vreinterpret_v_i32m4_u32m4(v_rs),
                              vl),
        vl);

    vint32m4_t rem = __riscv_vand_vv_i32m4(high, rmask, vl);

    vint32m4_t thresh
        = __riscv_vreinterpret_v_u32m4_i32m4(__riscv_vsrl_vx_u32m4(
            __riscv_vreinterpret_v_i32m4_u32m4(rmask), 1, vl));

    vbool8_t neg_mask = __riscv_vmslt_vx_i32m4_b8(high, 0, vl);
    thresh = __riscv_vadd_vx_i32m4_tum(neg_mask, thresh, thresh, 1, vl);

    vbool8_t round_up
        = __riscv_vmsgtu_vv_u32m4_b8(__riscv_vreinterpret_v_i32m4_u32m4(rem),
                                     __riscv_vreinterpret_v_i32m4_u32m4(thresh),
                                     vl);

    vint32m4_t result = __riscv_vsra_vv_i32m4(
        high, __riscv_vreinterpret_v_i32m4_u32m4(v_rs), vl);

    vint32m4_t result_p1 = __riscv_vadd_vx_i32m4(result, 1, vl);
    result = __riscv_vmerge_vvm_i32m4(result, result_p1, round_up, vl);

    return result;
}

static inline void
rvv_pack_activation_s8(vint32m4_t result, int8_t *out, size_t vl)
{
    result = __riscv_vadd_vx_i32m4(result, OUTPUT_OFFSET, vl);
    result = __riscv_vmax_vx_i32m4(result, OUT_ACT_MIN, vl);
    result = __riscv_vmin_vx_i32m4(result, OUT_ACT_MAX, vl);

    vint16m2_t r16 = __riscv_vnclip_wx_i16m2(result, 0, __RISCV_VXRM_RNU, vl);
    vint8m1_t  r8  = __riscv_vnclip_wx_i8m1(r16, 0, __RISCV_VXRM_RNU, vl);

    __riscv_vse8_v_i8m1(out, r8, vl);
}

static inline vint32m2_t
rvv_requantize_m2(vint32m2_t acc,
                  vint32m2_t v_mult,
                  vint32m2_t v_ls,
                  vint32m2_t v_rs,
                  size_t     vl)
{
    vint32m2_t pre = __riscv_vsll_vv_i32m2(
        acc, __riscv_vreinterpret_v_i32m2_u32m2(v_ls), vl);
    vint32m2_t high = __riscv_vsmul_vv_i32m2(pre, v_mult, __RISCV_VXRM_RNU, vl);

    vint32m2_t rmask = __riscv_vnot_v_i32m2(
        __riscv_vsll_vv_i32m2(__riscv_vmv_v_x_i32m2(-1, vl),
                              __riscv_vreinterpret_v_i32m2_u32m2(v_rs),
                              vl),
        vl);

    vint32m2_t rem = __riscv_vand_vv_i32m2(high, rmask, vl);

    vint32m2_t thresh
        = __riscv_vreinterpret_v_u32m2_i32m2(__riscv_vsrl_vx_u32m2(
            __riscv_vreinterpret_v_i32m2_u32m2(rmask), 1, vl));

    vbool16_t neg_mask = __riscv_vmslt_vx_i32m2_b16(high, 0, vl);
    thresh = __riscv_vadd_vx_i32m2_tum(neg_mask, thresh, thresh, 1, vl);

    vbool16_t round_up = __riscv_vmsgtu_vv_u32m2_b16(
        __riscv_vreinterpret_v_i32m2_u32m2(rem),
        __riscv_vreinterpret_v_i32m2_u32m2(thresh),
        vl);

    vint32m2_t result = __riscv_vsra_vv_i32m2(
        high, __riscv_vreinterpret_v_i32m2_u32m2(v_rs), vl);

    vint32m2_t result_p1 = __riscv_vadd_vx_i32m2(result, 1, vl);
    result = __riscv_vmerge_vvm_i32m2(result, result_p1, round_up, vl);

    return result;
}

static inline void
rvv_pack_activation_s8_m2(vint32m2_t result, int8_t *out, size_t vl)
{
    result = __riscv_vadd_vx_i32m2(result, OUTPUT_OFFSET, vl);
    result = __riscv_vmax_vx_i32m2(result, OUT_ACT_MIN, vl);
    result = __riscv_vmin_vx_i32m2(result, OUT_ACT_MAX, vl);

    vint16m1_t r16 = __riscv_vnclip_wx_i16m1(result, 0, __RISCV_VXRM_RNU, vl);
    vint8mf2_t r8  = __riscv_vnclip_wx_i8mf2(r16, 0, __RISCV_VXRM_RNU, vl);

    __riscv_vse8_v_i8mf2(out, r8, vl);
}

static void
rvv_process_pixel(const int8_t  *ip_col[KERNEL_DIM],
                  const int8_t  *kp_col[KERNEL_DIM],
                  int32_t        kh_start,
                  int32_t        kh_end,
                  const int32_t *bias,
                  const int32_t *output_mult,
                  const int32_t *output_shift,
                  int8_t        *out_pixel_ptr)
{
    int32_t ch = 0;

    int32_t kh0 = kh_start < 0 ? 0 : kh_start;
    int32_t kh1 = kh_end > KERNEL_DIM ? KERNEL_DIM : kh_end;

    while (ch < INPUT_CH)
    {
        size_t vl = __riscv_vsetvl_e32m4((size_t)(INPUT_CH - ch));

        vint32m4_t acc = __riscv_vle32_v_i32m4(bias + ch, vl);

        vint32m4_t v_mult  = __riscv_vle32_v_i32m4(output_mult + ch, vl);
        vint32m4_t v_shift = __riscv_vle32_v_i32m4(output_shift + ch, vl);

        vint32m4_t v_ls = __riscv_vmax_vx_i32m4(v_shift, 0, vl);
        vint32m4_t v_rs
            = __riscv_vneg_v_i32m4(__riscv_vmin_vx_i32m4(v_shift, 0, vl), vl);

        if (ip_col[0] != NULL)
        {
            const int8_t *ip = ip_col[0] + ch;
            const int8_t *kp = kp_col[0] + ch;
            for (int32_t r = kh0; r < kh1; ++r)
            {
                acc = rvv_acc_tap(
                    acc, ip + r * INP_ROW_STRIDE, kp + r * KER_ROW_STRIDE, vl);
            }
        }

        if (ip_col[1] != NULL)
        {
            const int8_t *ip = ip_col[1] + ch;
            const int8_t *kp = kp_col[1] + ch;
            for (int32_t r = kh0; r < kh1; ++r)
            {
                acc = rvv_acc_tap(
                    acc, ip + r * INP_ROW_STRIDE, kp + r * KER_ROW_STRIDE, vl);
            }
        }

        if (ip_col[2] != NULL)
        {
            const int8_t *ip = ip_col[2] + ch;
            const int8_t *kp = kp_col[2] + ch;
            for (int32_t r = kh0; r < kh1; ++r)
            {
                acc = rvv_acc_tap(
                    acc, ip + r * INP_ROW_STRIDE, kp + r * KER_ROW_STRIDE, vl);
            }
        }

        vint32m4_t res = rvv_requantize(acc, v_mult, v_ls, v_rs, vl);
        rvv_pack_activation_s8(res, out_pixel_ptr + ch, vl);

        ch += (int32_t)vl;
    }
}

int32_t
nn_depthwise_conv_3x3_s8(const nn_per_channel_quant_params *quant_params,
                         const q7_t                        *input,
                         const q7_t                        *kernel,
                         const int32_t                     *bias,
                         q7_t                              *output)
{
    const int32_t *output_mult  = quant_params->multiplier;
    const int32_t *output_shift = quant_params->shift;

    int32_t int_h0 = 0;
    while (int_h0 < OUTPUT_H && (int_h0 * STRIDE_H - PAD_H) < 0)
    {
        ++int_h0;
    }

    int32_t int_h1 = int_h0;
    while (int_h1 < OUTPUT_H && (int_h1 * STRIDE_H - PAD_H + 2) < INPUT_H)
    {
        ++int_h1;
    }

    int32_t int_w0 = 0;
    while (int_w0 < OUTPUT_W && (int_w0 * STRIDE_W - PAD_W) < 0)
    {
        ++int_w0;
    }

    int32_t int_w1 = int_w0;
    while (int_w1 < OUTPUT_W && (int_w1 * STRIDE_W - PAD_W + 2) < INPUT_W)
    {
        ++int_w1;
    }

#define PROCESS_BORDER_PIXEL(in_h_, in_w_, out_h_, out_w_, kh_start_, kh_end_) \
    do                                                                         \
    {                                                                          \
        const int8_t *_ip_col[KERNEL_DIM];                                     \
        const int8_t *_kp_col[KERNEL_DIM];                                     \
        for (int32_t _c = 0; _c < KERNEL_DIM; ++_c)                            \
        {                                                                      \
            int32_t _iw = (in_w_) + _c;                                        \
            if (_iw >= 0 && _iw < INPUT_W)                                     \
            {                                                                  \
                _ip_col[_c]                                                    \
                    = input + (in_h_) * INP_ROW_STRIDE + _iw * COL_STRIDE;     \
                _kp_col[_c] = kernel + _c * COL_STRIDE;                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                _ip_col[_c] = NULL;                                            \
                _kp_col[_c] = NULL;                                            \
            }                                                                  \
        }                                                                      \
        int8_t *_out_ptr                                                       \
            = output + ((out_h_) * OUTPUT_W + (out_w_)) * INPUT_CH;            \
        rvv_process_pixel(_ip_col,                                             \
                          _kp_col,                                             \
                          (kh_start_),                                         \
                          (kh_end_),                                           \
                          bias,                                                \
                          output_mult,                                         \
                          output_shift,                                        \
                          _out_ptr);                                           \
    } while (0)

    for (int32_t out_h = 0; out_h < int_h0; ++out_h)
    {
        const int32_t in_h     = out_h * STRIDE_H - PAD_H;
        const int32_t kh_start = -in_h;
        const int32_t kh_end   = MIN(KERNEL_DIM, INPUT_H - in_h);

        for (int32_t out_w = 0; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w = out_w * STRIDE_W - PAD_W;
            PROCESS_BORDER_PIXEL(in_h, in_w, out_h, out_w, kh_start, kh_end);
        }
    }

    for (int32_t out_h = int_h0; out_h < int_h1; ++out_h)
    {
        const int32_t in_h = out_h * STRIDE_H - PAD_H;

        for (int32_t out_w = 0; out_w < int_w0; ++out_w)
        {
            const int32_t in_w = out_w * STRIDE_W - PAD_W;
            PROCESS_BORDER_PIXEL(in_h, in_w, out_h, out_w, 0, KERNEL_DIM);
        }

        for (int32_t out_w = int_w1; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w = out_w * STRIDE_W - PAD_W;
            PROCESS_BORDER_PIXEL(in_h, in_w, out_h, out_w, 0, KERNEL_DIM);
        }
    }

    for (int32_t out_h = int_h1; out_h < OUTPUT_H; ++out_h)
    {
        const int32_t in_h   = out_h * STRIDE_H - PAD_H;
        const int32_t kh_end = MIN(KERNEL_DIM, INPUT_H - in_h);

        for (int32_t out_w = 0; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w = out_w * STRIDE_W - PAD_W;
            PROCESS_BORDER_PIXEL(in_h, in_w, out_h, out_w, 0, kh_end);
        }
    }

    if (int_h0 < int_h1 && int_w0 < int_w1)
    {
        const int32_t in_h0_val = int_h0 * STRIDE_H - PAD_H;
        const int32_t in_w0_val = int_w0 * STRIDE_W - PAD_W;
        const int8_t *ip_base
            = input + in_h0_val * INP_ROW_STRIDE + in_w0_val * COL_STRIDE;

        int32_t ch = 0;
        while (ch < INPUT_CH)
        {
            size_t vl = __riscv_vsetvl_e32m2((size_t)(INPUT_CH - ch));

            vint32m2_t v_bias  = __riscv_vle32_v_i32m2(bias + ch, vl);
            vint32m2_t v_mult  = __riscv_vle32_v_i32m2(output_mult + ch, vl);
            vint32m2_t v_shift = __riscv_vle32_v_i32m2(output_shift + ch, vl);
            vint32m2_t v_ls    = __riscv_vmax_vx_i32m2(v_shift, 0, vl);
            vint32m2_t v_rs    = __riscv_vneg_v_i32m2(
                __riscv_vmin_vx_i32m2(v_shift, 0, vl), vl);

#define LOAD_K_TAP(row, col)                                                \
    __riscv_vsext_vf2_i16m1(                                                \
        __riscv_vle8_v_i8mf2(                                               \
            kernel + (row) * KER_ROW_STRIDE + (col) * COL_STRIDE + ch, vl), \
        vl)

            vint16m1_t k00 = LOAD_K_TAP(0, 0);
            vint16m1_t k01 = LOAD_K_TAP(0, 1);
            vint16m1_t k02 = LOAD_K_TAP(0, 2);

            vint16m1_t k10 = LOAD_K_TAP(1, 0);
            vint16m1_t k11 = LOAD_K_TAP(1, 1);
            vint16m1_t k12 = LOAD_K_TAP(1, 2);

            vint16m1_t k20 = LOAD_K_TAP(2, 0);
            vint16m1_t k21 = LOAD_K_TAP(2, 1);
            vint16m1_t k22 = LOAD_K_TAP(2, 2);

#undef LOAD_K_TAP

            vint16m1_t k_sum = __riscv_vadd_vv_i16m1(k00, k01, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k02, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k10, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k11, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k12, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k20, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k21, vl);
            k_sum            = __riscv_vadd_vv_i16m1(k_sum, k22, vl);

            vint32m2_t v_bias_folded = __riscv_vwmacc_vx_i32m2(
                v_bias, (int16_t)INPUT_OFFSET, k_sum, vl);

            const int8_t *ip_h = ip_base + ch;
            int32_t       out_pixel_idx_h
                = (int_h0 * OUTPUT_W + int_w0) * INPUT_CH + ch;

            for (int32_t out_h = int_h0; out_h < int_h1; ++out_h)
            {
                const int8_t *ip_w          = ip_h;
                int32_t       out_pixel_idx = out_pixel_idx_h;

                for (int32_t out_w = int_w0; out_w < int_w1; ++out_w)
                {
                    vint32m2_t acc = v_bias_folded;

                    const int8_t *in0 = ip_w;
                    const int8_t *in1 = ip_w + INP_ROW_STRIDE;
                    const int8_t *in2 = ip_w + 2 * INP_ROW_STRIDE;

#define ACC_TAP(in_ptr, col_idx, k_reg)                                       \
    acc = __riscv_vwmacc_vv_i32m2(                                            \
        acc,                                                                  \
        __riscv_vsext_vf2_i16m1(                                              \
            __riscv_vle8_v_i8mf2((in_ptr) + (col_idx) * COL_STRIDE, vl), vl), \
        (k_reg),                                                              \
        vl)

                    ACC_TAP(in0, 0, k00);
                    ACC_TAP(in0, 1, k01);
                    ACC_TAP(in0, 2, k02);

                    ACC_TAP(in1, 0, k10);
                    ACC_TAP(in1, 1, k11);
                    ACC_TAP(in1, 2, k12);

                    ACC_TAP(in2, 0, k20);
                    ACC_TAP(in2, 1, k21);
                    ACC_TAP(in2, 2, k22);

#undef ACC_TAP

                    vint32m2_t res
                        = rvv_requantize_m2(acc, v_mult, v_ls, v_rs, vl);
                    rvv_pack_activation_s8_m2(res, output + out_pixel_idx, vl);

                    ip_w += INPUT_X_STEP;
                    out_pixel_idx += INPUT_CH;
                }
                ip_h += STRIDE_H * INP_ROW_STRIDE;
                out_pixel_idx_h += OUTPUT_W * INPUT_CH;
            }
            ch += (int32_t)vl;
        }
    }

#undef PROCESS_BORDER_PIXEL

    return 0;
}
