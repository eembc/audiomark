/* Copyright (C) 2003 Epic Games (written by Jean-Marc Valin)
   Copyright (C) 2004-2006 Epic Games
   Copyright (C) 2026 Harshit Kumar Shivhare

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef FLOATING_POINT

#include "arch.h"
#include <riscv_vector.h>

#ifdef OVERRIDE_MDF_INNER_PROD
static spx_word32_t
mdf_inner_prod(const spx_word16_t *x, const spx_word16_t *y, int len)
{
    const float *_x = (const float *)x;
    const float *_y = (const float *)y;

    size_t       vlmax = __riscv_vsetvlmax_e32m8();
    vfloat32m8_t v_s   = __riscv_vfmv_v_f_f32m8(0.0f, vlmax);

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_x = __riscv_vle32_v_f32m8(_x, vl);
        vfloat32m8_t v_y = __riscv_vle32_v_f32m8(_y, vl);

        v_s = __riscv_vfmacc_vv_f32m8_tu(v_s, v_x, v_y, vl);

        len -= vl;
        _x += vl;
        _y += vl;
    }

    size_t       vlmax_m1 = __riscv_vsetvlmax_e32m1();
    vfloat32m1_t v_zero   = __riscv_vfmv_v_f_f32m1(0.0f, vlmax_m1);
    vfloat32m1_t v_sum = __riscv_vfredusum_vs_f32m8_f32m1(v_s, v_zero, vlmax);

    return (spx_word32_t)__riscv_vfmv_f_s_f32m1_f32(v_sum);
}
#endif

#ifdef OVERRIDE_MDF_VEC_SUB
static void
vect_sub(const spx_word16_t *pSrcA,
         const spx_word16_t *pSrcB,
         spx_word16_t       *pDst,
         uint32_t            blockSize)
{
    uint32_t len = blockSize;
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_a = __riscv_vle32_v_f32m8((const float *)pSrcA, vl);
        vfloat32m8_t v_b = __riscv_vle32_v_f32m8((const float *)pSrcB, vl);

        vfloat32m8_t v_result = __riscv_vfsub_vv_f32m8(v_a, v_b, vl);

        __riscv_vse32_v_f32m8((float *)pDst, v_result, vl);

        len -= vl;
        pSrcA += vl;
        pSrcB += vl;
        pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_VEC_SUB_INT16
static void
vect_sub16(const spx_int16_t *pSrcA,
           const spx_int16_t *pSrcB,
           spx_word16_t      *pDst,
           uint32_t           blockSize)
{
    uint32_t len = blockSize;
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e16m4(len);

        vint16m4_t v_a = __riscv_vle16_v_i16m4(pSrcA, vl);
        vint16m4_t v_b = __riscv_vle16_v_i16m4(pSrcB, vl);

        vint32m8_t   v_diff   = __riscv_vwsub_vv_i32m8(v_a, v_b, vl);
        vfloat32m8_t v_result = __riscv_vfcvt_f_x_v_f32m8(v_diff, vl);

        __riscv_vse32_v_f32m8((float *)pDst, v_result, vl);

        len -= vl;
        pSrcA += vl;
        pSrcB += vl;
        pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_VEC_ADD
static void
vect_add(const spx_word16_t *pSrcA,
         const spx_word16_t *pSrcB,
         spx_word16_t       *pDst,
         uint32_t            blockSize)
{
    uint32_t len = blockSize;
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_a = __riscv_vle32_v_f32m8((const float *)pSrcA, vl);
        vfloat32m8_t v_b = __riscv_vle32_v_f32m8((const float *)pSrcB, vl);

        vfloat32m8_t v_result = __riscv_vfadd_vv_f32m8(v_a, v_b, vl);

        __riscv_vse32_v_f32m8((float *)pDst, v_result, vl);

        len -= vl;
        pSrcA += vl;
        pSrcB += vl;
        pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_VEC_MULT
static void
vect_mult(const spx_word16_t *pSrcA,
          const spx_word16_t *pSrcB,
          spx_word16_t       *pDst,
          uint32_t            blockSize)
{
    uint32_t len = blockSize;
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_a = __riscv_vle32_v_f32m8((const float *)pSrcA, vl);
        vfloat32m8_t v_b = __riscv_vle32_v_f32m8((const float *)pSrcB, vl);

        vfloat32m8_t v_result = __riscv_vfmul_vv_f32m8(v_a, v_b, vl);

        __riscv_vse32_v_f32m8((float *)pDst, v_result, vl);

        len -= vl;
        pSrcA += vl;
        pSrcB += vl;
        pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_VEC_SCALE
static void
vect_scale(const spx_word16_t *pSrc,
           spx_word16_t        scale,
           spx_word16_t       *pDst,
           uint32_t            blockSize)
{
    uint32_t len    = blockSize;
    float    _scale = (float)scale;

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_a = __riscv_vle32_v_f32m8((const float *)pSrc, vl);

        vfloat32m8_t v_result = __riscv_vfmul_vf_f32m8(v_a, _scale, vl);
        __riscv_vse32_v_f32m8((float *)pDst, v_result, vl);

        len -= vl;
        pSrc += vl;
        pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_SMOOTHED_ADD
static void
smoothed_add(const spx_word16_t *pSrc1,
             const spx_word16_t *pWin1,
             const spx_word16_t *pSrc2,
             const spx_word16_t *pWin2,
             spx_word16_t       *pDst,
             uint16_t            frame_size,
             uint16_t            nbChan,
             uint16_t            N)
{
    for (int chan = 0; chan < nbChan; chan++)
    {
        uint32_t len = frame_size;

        const float *_pSrc1 = (const float *)pSrc1 + (chan * N);
        const float *_pSrc2 = (const float *)pSrc2 + (chan * N);
        float       *_pDst  = (float *)pDst + (chan * N);

        const float *_pWin1 = (const float *)pWin1;
        const float *_pWin2 = (const float *)pWin2;

        while (len > 0)
        {
            size_t vl = __riscv_vsetvl_e32m8(len);

            vfloat32m8_t v_src1 = __riscv_vle32_v_f32m8(_pSrc1, vl);
            vfloat32m8_t v_win1 = __riscv_vle32_v_f32m8(_pWin1, vl);
            vfloat32m8_t v_tmp  = __riscv_vfmul_vv_f32m8(v_src1, v_win1, vl);

            vfloat32m8_t v_src2 = __riscv_vle32_v_f32m8(_pSrc2, vl);
            vfloat32m8_t v_win2 = __riscv_vle32_v_f32m8(_pWin2, vl);
            v_tmp = __riscv_vfmacc_vv_f32m8(v_tmp, v_src2, v_win2, vl);

            __riscv_vse32_v_f32m8(_pDst, v_tmp, vl);

            len -= vl;
            _pSrc1 += vl;
            _pSrc2 += vl;
            _pWin1 += vl;
            _pWin2 += vl;
            _pDst += vl;
        }
    }
}
#endif

#ifdef OVERRIDE_MDF_SMOOTH_FE_NRG
static void
smooth_fe_nrg(spx_word32_t *in1,
              spx_word16_t  c1,
              spx_word32_t *in2,
              spx_word16_t  c2,
              spx_word32_t *pDst,
              uint16_t      frame_size)
{
    uint32_t len = frame_size;

    const float *_in1  = (const float *)in1;
    const float *_in2  = (const float *)in2;
    float       *_pDst = (float *)pDst;

    float _c1 = (float)c1;
    float _c2 = (float)c2;

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_dst = __riscv_vfmv_v_f_f32m8(1.0f, vl);
        vfloat32m8_t v_in1 = __riscv_vle32_v_f32m8(_in1, vl);
        vfloat32m8_t v_in2 = __riscv_vle32_v_f32m8(_in2, vl);

        v_dst = __riscv_vfmacc_vf_f32m8(v_dst, _c1, v_in1, vl);
        v_dst = __riscv_vfmacc_vf_f32m8(v_dst, _c2, v_in2, vl);

        __riscv_vse32_v_f32m8(_pDst, v_dst, vl);

        len -= vl;
        _in1 += vl;
        _in2 += vl;
        _pDst += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC
static void
mdf_non_adapt_learning_rate_calc(spx_word32_t *power,
                                 spx_float_t  *power_1,
                                 spx_word16_t  adapt_rate,
                                 uint16_t      frame_size)
{
    uint32_t len = frame_size;

    const float *_power      = (const float *)power;
    float       *_power_1    = (float *)power_1;
    float        _adapt_rate = (float)adapt_rate;

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_pwr = __riscv_vle32_v_f32m8(_power, vl);
        v_pwr              = __riscv_vfadd_vf_f32m8(v_pwr, 10.0f, vl);

        /* INFO: it might be possible to somehow remove vfrdiv using vfrec7 */
        vfloat32m8_t v_res = __riscv_vfrdiv_vf_f32m8(v_pwr, _adapt_rate, vl);

        __riscv_vse32_v_f32m8(_power_1, v_res, vl);

        len -= vl;
        _power += vl;
        _power_1 += vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_POWER_SPECTRUM
static void
power_spectrum(const spx_word16_t *X, spx_word32_t *ps, int N)
{
    const float *_X  = (const float *)X;
    float       *_ps = (float *)ps;

    _ps[0] = _X[0] * _X[0];

    int          num_bins = (N - 2) / 2;
    const float *pX_cplx  = _X + 1;
    float       *pPS      = _ps + 1;

    while (num_bins > 0)
    {
        size_t         vl   = __riscv_vsetvl_e32m4(num_bins);
        vfloat32m4x2_t seg  = __riscv_vlseg2e32_v_f32m4x2(pX_cplx, vl);
        vfloat32m4_t   v_re = __riscv_vget_v_f32m4x2_f32m4(seg, 0);
        vfloat32m4_t   v_im = __riscv_vget_v_f32m4x2_f32m4(seg, 1);

        vfloat32m4_t v_mag_sq = __riscv_vfmul_vv_f32m4(v_re, v_re, vl);
        v_mag_sq = __riscv_vfmacc_vv_f32m4(v_mag_sq, v_im, v_im, vl);

        __riscv_vse32_v_f32m4(pPS, v_mag_sq, vl);

        num_bins -= vl;
        pX_cplx += 2 * vl;
        pPS += vl;
    }

    _ps[N / 2] = _X[N - 1] * _X[N - 1];
}
#endif

#ifdef OVERRIDE_MDF_POWER_SPECTRUM_ACCUM
static void
power_spectrum_accum(const spx_word16_t *X, spx_word32_t *ps, int N)
{
    const float *_X  = (const float *)X;
    float       *_ps = (float *)ps;

    _ps[0] += _X[0] * _X[0];

    int          num_bins = (N - 2) / 2;
    const float *pX_cplx  = _X + 1;
    float       *pPS      = _ps + 1;

    while (num_bins > 0)
    {
        size_t         vl   = __riscv_vsetvl_e32m4(num_bins);
        vfloat32m4x2_t seg  = __riscv_vlseg2e32_v_f32m4x2(pX_cplx, vl);
        vfloat32m4_t   v_re = __riscv_vget_v_f32m4x2_f32m4(seg, 0);
        vfloat32m4_t   v_im = __riscv_vget_v_f32m4x2_f32m4(seg, 1);

        vfloat32m4_t v_mag_sq = __riscv_vfmul_vv_f32m4(v_re, v_re, vl);
        v_mag_sq = __riscv_vfmacc_vv_f32m4(v_mag_sq, v_im, v_im, vl);

        vfloat32m4_t v_existing = __riscv_vle32_v_f32m4(pPS, vl);
        vfloat32m4_t v_sum = __riscv_vfadd_vv_f32m4(v_existing, v_mag_sq, vl);

        __riscv_vse32_v_f32m4(pPS, v_sum, vl);

        num_bins -= vl;
        pX_cplx += 2 * vl;
        pPS += vl;
    }

    _ps[N / 2] += _X[N - 1] * _X[N - 1];
}
#endif

static inline void
riscv_rvv_memset_zero(float *dst, size_t len)
{
    size_t       vlmax = __riscv_vsetvlmax_e32m8();
    vfloat32m8_t vzero = __riscv_vfmv_v_f_f32m8(0.0f, vlmax);
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);
        __riscv_vse32_v_f32m8(dst, vzero, vl);
        dst += vl;
        len -= vl;
    }
}

#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM
static void
spectral_mul_accum(const spx_word16_t *X,
                   const spx_word32_t *Y,
                   spx_word16_t       *acc,
                   int                 N,
                   int                 M)
{
    const float *_X   = (const float *)X;
    const float *_Y   = (const float *)Y;
    float       *_acc = (float *)acc;

    riscv_rvv_memset_zero(_acc, N);

    size_t          vlmax  = __riscv_vsetvlmax_e32m4();
    const ptrdiff_t stride = 2 * sizeof(float);

    for (int j = 0; j < M; j++)
    {
        _acc[0] += _X[0] * _Y[0];

        int          num_bins  = (N - 2) / 2;
        const float *pX_cplx   = _X + 1;
        const float *pY_cplx   = _Y + 1;
        float       *pAcc_cplx = _acc + 1;

        while (num_bins >= (int)vlmax)
        {
            vfloat32m4x2_t seg_x = __riscv_vlseg2e32_v_f32m4x2(pX_cplx, vlmax);
            vfloat32m4_t   v_xr  = __riscv_vget_v_f32m4x2_f32m4(seg_x, 0);
            vfloat32m4_t   v_xi  = __riscv_vget_v_f32m4x2_f32m4(seg_x, 1);

            vfloat32m4x2_t seg_y = __riscv_vlseg2e32_v_f32m4x2(pY_cplx, vlmax);
            vfloat32m4_t   v_yr  = __riscv_vget_v_f32m4x2_f32m4(seg_y, 0);
            vfloat32m4_t   v_yi  = __riscv_vget_v_f32m4x2_f32m4(seg_y, 1);

            vfloat32m4x2_t seg_acc
                = __riscv_vlseg2e32_v_f32m4x2(pAcc_cplx, vlmax);
            vfloat32m4_t v_accr = __riscv_vget_v_f32m4x2_f32m4(seg_acc, 0);
            vfloat32m4_t v_acci = __riscv_vget_v_f32m4x2_f32m4(seg_acc, 1);

            v_accr = __riscv_vfmacc_vv_f32m4(v_accr, v_xr, v_yr, vlmax);
            v_accr = __riscv_vfnmsac_vv_f32m4(v_accr, v_xi, v_yi, vlmax);

            v_acci = __riscv_vfmacc_vv_f32m4(v_acci, v_xi, v_yr, vlmax);
            v_acci = __riscv_vfmacc_vv_f32m4(v_acci, v_xr, v_yi, vlmax);

            seg_acc = __riscv_vset_v_f32m4_f32m4x2(seg_acc, 0, v_accr);
            seg_acc = __riscv_vset_v_f32m4_f32m4x2(seg_acc, 1, v_acci);
            __riscv_vsseg2e32_v_f32m4x2(pAcc_cplx, seg_acc, vlmax);

            pX_cplx += 2 * vlmax;
            pY_cplx += 2 * vlmax;
            pAcc_cplx += 2 * vlmax;
            num_bins -= vlmax;
        }

        if (num_bins > 0)
        {
            size_t vl = __riscv_vsetvl_e32m4(num_bins);

            vfloat32m4_t v_xr = __riscv_vlse32_v_f32m4(pX_cplx, stride, vl);
            vfloat32m4_t v_xi = __riscv_vlse32_v_f32m4(pX_cplx + 1, stride, vl);

            vfloat32m4_t v_yr = __riscv_vlse32_v_f32m4(pY_cplx, stride, vl);
            vfloat32m4_t v_yi = __riscv_vlse32_v_f32m4(pY_cplx + 1, stride, vl);

            vfloat32m4_t v_accr = __riscv_vlse32_v_f32m4(pAcc_cplx, stride, vl);
            vfloat32m4_t v_acci
                = __riscv_vlse32_v_f32m4(pAcc_cplx + 1, stride, vl);

            v_accr = __riscv_vfmacc_vv_f32m4(v_accr, v_xr, v_yr, vl);
            v_accr = __riscv_vfnmsac_vv_f32m4(v_accr, v_xi, v_yi, vl);

            v_acci = __riscv_vfmacc_vv_f32m4(v_acci, v_xi, v_yr, vl);
            v_acci = __riscv_vfmacc_vv_f32m4(v_acci, v_xr, v_yi, vl);

            __riscv_vsse32_v_f32m4(pAcc_cplx, stride, v_accr, vl);
            __riscv_vsse32_v_f32m4(pAcc_cplx + 1, stride, v_acci, vl);
        }

        _acc[N - 1] += _X[N - 1] * _Y[N - 1];

        _X += N;
        _Y += N;
    }
}
#endif

#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM16
#define spectral_mul_accum16 spectral_mul_accum
#endif

#ifdef OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ
static void
weighted_spectral_mul_conj(const spx_float_t  *w,
                           const spx_float_t   p,
                           const spx_word16_t *X,
                           const spx_word16_t *Y,
                           spx_word32_t       *prod,
                           int                 N)
{
    const float *_w    = (const float *)w;
    const float *_X    = (const float *)X;
    const float *_Y    = (const float *)Y;
    float       *_prod = (float *)prod;
    float        _p    = (float)p;

    _prod[0] = _p * _w[0] * _X[0] * _Y[0];

    int          num_bins   = (N - 2) / 2;
    const float *pw         = _w + 1;
    const float *pX_cplx    = _X + 1;
    const float *pY_cplx    = _Y + 1;
    float       *pProd_cplx = _prod + 1;

    while (num_bins > 0)
    {
        size_t       vl  = __riscv_vsetvl_e32m4(num_bins);
        vfloat32m4_t v_w = __riscv_vle32_v_f32m4(pw, vl);
        v_w              = __riscv_vfmul_vf_f32m4(v_w, _p, vl);

        vfloat32m4x2_t seg_x = __riscv_vlseg2e32_v_f32m4x2(pX_cplx, vl);
        vfloat32m4_t   v_xr  = __riscv_vget_v_f32m4x2_f32m4(seg_x, 0);
        vfloat32m4_t   v_xi  = __riscv_vget_v_f32m4x2_f32m4(seg_x, 1);

        vfloat32m4x2_t seg_y = __riscv_vlseg2e32_v_f32m4x2(pY_cplx, vl);
        vfloat32m4_t   v_yr  = __riscv_vget_v_f32m4x2_f32m4(seg_y, 0);
        vfloat32m4_t   v_yi  = __riscv_vget_v_f32m4x2_f32m4(seg_y, 1);

        vfloat32m4_t v_re = __riscv_vfmul_vv_f32m4(v_xr, v_yr, vl);
        v_re              = __riscv_vfmacc_vv_f32m4(v_re, v_xi, v_yi, vl);
        v_re              = __riscv_vfmul_vv_f32m4(v_re, v_w, vl);

        vfloat32m4_t v_im = __riscv_vfmul_vv_f32m4(v_xr, v_yi, vl);
        v_im              = __riscv_vfnmsac_vv_f32m4(v_im, v_xi, v_yr, vl);
        v_im              = __riscv_vfmul_vv_f32m4(v_im, v_w, vl);

        vfloat32m4x2_t seg_prod = __riscv_vundefined_f32m4x2();
        seg_prod = __riscv_vset_v_f32m4_f32m4x2(seg_prod, 0, v_re);
        seg_prod = __riscv_vset_v_f32m4_f32m4x2(seg_prod, 1, v_im);
        __riscv_vsseg2e32_v_f32m4x2(pProd_cplx, seg_prod, vl);

        num_bins -= vl;
        pw += vl;
        pX_cplx += 2 * vl;
        pY_cplx += 2 * vl;
        pProd_cplx += 2 * vl;
    }

    _prod[N - 1] = _p * _w[N / 2] * _X[N - 1] * _Y[N - 1];
}
#endif

#ifdef OVERRIDE_MDF_ADJUST_PROP
static void
mdf_adjust_prop(const spx_word32_t *W, int N, int M, int P, spx_word16_t *prop)
{
    const float *_W    = (const float *)W;
    float       *_prop = (float *)prop;

    float max_sum  = 1.0f;
    float prop_sum = 1.0f;

    size_t vlmax = __riscv_vsetvlmax_e32m8();

    for (int i = 0; i < M; i++)
    {
        vfloat32m8_t v_acc = __riscv_vfmv_v_f_f32m8(0.0f, vlmax);

        for (int p = 0; p < P; p++)
        {
            const float *pW  = &_W[p * N * M + i * N];
            int          len = N;

            while (len > 0)
            {
                size_t       vl  = __riscv_vsetvl_e32m8(len);
                vfloat32m8_t v_w = __riscv_vle32_v_f32m8(pW, vl);
                v_acc = __riscv_vfmacc_vv_f32m8_tu(v_acc, v_w, v_w, vl);

                pW += vl;
                len -= vl;
            }
        }

        vfloat32m1_t v_red
            = __riscv_vfmv_s_f_f32m1(0.0f, __riscv_vsetvlmax_e32m1());
        v_red = __riscv_vfredusum_vs_f32m8_f32m1(v_acc, v_red, vlmax);

        float tmp = 1.0f + __riscv_vfmv_f_s_f32m1_f32(v_red);

        _prop[i] = sqrtf(tmp);
        if (_prop[i] > max_sum)
        {
            max_sum = _prop[i];
        }
    }

    for (int i = 0; i < M; i++)
    {
        _prop[i] += 0.1f * max_sum;
        prop_sum += _prop[i];
    }

    float norm_factor = 0.99f / prop_sum;
    for (int i = 0; i < M; i++)
    {
        _prop[i] *= norm_factor;
    }
}
#endif

#ifdef OVERRIDE_MDF_PREEMPH_FLT
static int
mdf_preemph(spx_word16_t *in,
            spx_word16_t *out,
            spx_word16_t  preemph,
            int           len,
            spx_word16_t *mem)
{
    const float *_in      = (const float *)in;
    float       *_out     = (float *)out;
    float        _preemph = (float)preemph;
    float        _mem     = (float)(*mem);

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m4(len);

        vfloat32m4_t v_in = __riscv_vle32_v_f32m4(_in, vl);

        /* INFO: it shifs the elements to the right and drops the
          last element in the last position */
        vfloat32m4_t v_prev = __riscv_vfslide1up_vf_f32m4(v_in, _mem, vl);

        _mem = _in[vl - 1];

        vfloat32m4_t v_out
            = __riscv_vfnmsac_vf_f32m4(v_in, _preemph, v_prev, vl);

        __riscv_vse32_v_f32m4(_out, v_out, vl);

        _in += vl;
        _out += vl;
        len -= vl;
    }

    *mem = (spx_word16_t)_mem;

    return 0;
}
#endif

#ifdef OVERRIDE_MDF_STRIDED_PREEMPH_FLT
static int
mdf_preemph_with_stride_int(const spx_int16_t *in,
                            spx_word16_t      *out,
                            spx_word16_t       preemph,
                            int                len,
                            spx_word16_t      *mem,
                            int                stride)
{
    float *_out     = (float *)out;
    float  _preemph = (float)preemph;
    float  _mem     = (float)(*mem);

    ptrdiff_t bstride = stride * sizeof(spx_int16_t);

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m4(len);

        vint16m2_t   v_in16 = __riscv_vlse16_v_i16m2(in, bstride, vl);
        vfloat32m4_t v_in   = __riscv_vfwcvt_f_x_v_f32m4(v_in16, vl);
        vfloat32m4_t v_prev = __riscv_vfslide1up_vf_f32m4(v_in, _mem, vl);

        _mem = (float)in[(vl - 1) * stride];

        vfloat32m4_t v_out
            = __riscv_vfnmsac_vf_f32m4(v_in, _preemph, v_prev, vl);

        __riscv_vse32_v_f32m4(_out, v_out, vl);

        in += vl * stride;
        _out += vl;
        len -= vl;
    }

    *mem = (spx_word16_t)_mem;

    return 0;
}
#endif

#ifdef OVERRIDE_MDF_NORM_LEARN_RATE_CALC
static void
mdf_nominal_learning_rate_calc(spx_word32_t *pRf,
                               spx_word32_t *power,
                               spx_word32_t *pYf,
                               spx_float_t  *power_1,
                               spx_word16_t  leak_estimate,
                               spx_word16_t  RER,
                               uint16_t      frame_size)
{
    const float *_pRf     = (const float *)pRf;
    const float *_power   = (const float *)power;
    const float *_pYf     = (const float *)pYf;
    float       *_power_1 = (float *)power_1;

    float _leak_estimate = (float)leak_estimate;
    float _RER           = (float)RER;

    float _rer_03 = 0.3f * _RER;

    uint32_t len = frame_size;

    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m8(len);

        vfloat32m8_t v_yf    = __riscv_vle32_v_f32m8(_pYf, vl);
        vfloat32m8_t v_rf    = __riscv_vle32_v_f32m8(_pRf, vl);
        vfloat32m8_t v_power = __riscv_vle32_v_f32m8(_power, vl);

        vfloat32m8_t v_r = __riscv_vfmul_vf_f32m8(v_yf, _leak_estimate, vl);

        vfloat32m8_t v_e = __riscv_vfadd_vf_f32m8(v_rf, 1.0f, vl);

        vfloat32m8_t v_e_half = __riscv_vfmul_vf_f32m8(v_e, 0.5f, vl);
        v_r                   = __riscv_vfmin_vv_f32m8(v_r, v_e_half, vl);

        vfloat32m8_t v_tmp = __riscv_vfmul_vf_f32m8(v_e, _rer_03, vl);
        v_r                = __riscv_vfmacc_vf_f32m8(v_tmp, 0.7f, v_r, vl);

        vfloat32m8_t v_pwr_plus_10 = __riscv_vfadd_vf_f32m8(v_power, 10.0f, vl);

        vfloat32m8_t v_den = __riscv_vfmul_vv_f32m8(v_e, v_pwr_plus_10, vl);

        vfloat32m8_t v_res = __riscv_vfdiv_vv_f32m8(v_r, v_den, vl);

        __riscv_vse32_v_f32m8(_power_1, v_res, vl);

        _pRf += vl;
        _power += vl;
        _pYf += vl;
        _power_1 += vl;
        len -= vl;
    }
}
#endif

#ifdef OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR
static void
filtered_spectra_cross_corr(spx_word32_t *pRf,
                            spx_word32_t *pEh,
                            spx_word32_t *pYf,
                            spx_word32_t *pYh,
                            spx_float_t  *Pey,
                            spx_float_t  *Pyy,
                            spx_word16_t  spec_average,
                            uint16_t      frame_size)
{
    const float *_pRf = (const float *)pRf;
    float       *_pEh = (float *)pEh;
    const float *_pYf = (const float *)pYf;
    float       *_pYh = (float *)pYh;

    float _spec_average  = (float)spec_average;
    float _spec_avg_comp = 1.0f - _spec_average;

    uint32_t len   = frame_size + 1;
    size_t   vlmax = __riscv_vsetvlmax_e32m4();

    vfloat32m4_t v_sum_pey = __riscv_vfmv_v_f_f32m4(0.0f, vlmax);
    vfloat32m4_t v_sum_pyy = __riscv_vfmv_v_f_f32m4(0.0f, vlmax);

    while (len > 0)
    {
        size_t       vl   = __riscv_vsetvl_e32m4(len);
        vfloat32m4_t v_rf = __riscv_vle32_v_f32m4(_pRf, vl);
        vfloat32m4_t v_eh = __riscv_vle32_v_f32m4(_pEh, vl);
        vfloat32m4_t v_yf = __riscv_vle32_v_f32m4(_pYf, vl);
        vfloat32m4_t v_yh = __riscv_vle32_v_f32m4(_pYh, vl);

        vfloat32m4_t v_eh_diff = __riscv_vfsub_vv_f32m4(v_rf, v_eh, vl);
        vfloat32m4_t v_yh_diff = __riscv_vfsub_vv_f32m4(v_yf, v_yh, vl);

        v_sum_pey
            = __riscv_vfmacc_vv_f32m4_tu(v_sum_pey, v_eh_diff, v_yh_diff, vl);
        v_sum_pyy
            = __riscv_vfmacc_vv_f32m4_tu(v_sum_pyy, v_yh_diff, v_yh_diff, vl);

        vfloat32m4_t v_eh_new = __riscv_vfmacc_vf_f32m4(
            __riscv_vfmul_vf_f32m4(v_eh, _spec_avg_comp, vl),
            _spec_average,
            v_rf,
            vl);
        vfloat32m4_t v_yh_new = __riscv_vfmacc_vf_f32m4(
            __riscv_vfmul_vf_f32m4(v_yh, _spec_avg_comp, vl),
            _spec_average,
            v_yf,
            vl);

        __riscv_vse32_v_f32m4(_pEh, v_eh_new, vl);
        __riscv_vse32_v_f32m4(_pYh, v_yh_new, vl);

        _pRf += vl;
        _pEh += vl;
        _pYf += vl;
        _pYh += vl;
        len -= vl;
    }

    size_t       vlmax_m1 = __riscv_vsetvlmax_e32m1();
    vfloat32m1_t v_red    = __riscv_vfmv_s_f_f32m1(0.0f, vlmax_m1);
    *Pey += __riscv_vfmv_f_s_f32m1_f32(
        __riscv_vfredusum_vs_f32m4_f32m1(v_sum_pey, v_red, vlmax));
    *Pyy += __riscv_vfmv_f_s_f32m1_f32(
        __riscv_vfredusum_vs_f32m4_f32m1(v_sum_pyy, v_red, vlmax));
}
#endif

#else

#error "FIXED_POINT is not implemented for mdf_opt_rvv"
#endif
