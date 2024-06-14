/* Copyright (C) 2003 Epic Games (written by Jean-Marc Valin)
   Copyright (C) 2004-2006 Epic Games

   File: preprocess.c
   Preprocessor with denoising based on the algorithm by Ephraim and Malah

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


#include <stdint.h>

/*
 * Reference code for optimized routines
 */


#ifdef OVERRIDE_MDF_DC_NOTCH
static void filter_dc_notch16(const spx_int16_t * in, spx_word16_t radius, spx_word16_t * out, int len, spx_mem_t * mem, int stride)
{
    int             i;
    spx_word16_t    den2;
#ifdef FIXED_POINT
    den2 = MULT16_16_Q15(radius, radius) + MULT16_16_Q15(QCONST16(.7f, 15), MULT16_16_Q15(32767 - radius, 32767 - radius));
#else
    den2 = radius * radius + .7f * (1.0f - radius) * (1.0f - radius);
#endif
    /*printf ("%d %d %d %d %d %d\n", num[0], num[1], num[2], den[0], den[1], den[2]); */
    for (i = 0; i < len; i++) {
        spx_word16_t    vin = in[i * stride];
        spx_word32_t    vout = mem[0] + SHL32(EXTEND32(vin), 15);
#ifdef FIXED_POINT
        mem[0] = mem[1] + SHL32(SHL32(-EXTEND32(vin), 15) + MULT16_32_Q15(radius, vout), 1);
#else
        mem[0] = mem[1] + 2.0f * (-vin + radius * vout);
#endif
        mem[1] = SHL32(EXTEND32(vin), 15) - MULT16_32_Q15(den2, vout);
        out[i] = SATURATE32(PSHR32(MULT16_32_Q15(radius, vout), 15), 32767);
    }
}

#endif

#ifdef OVERRIDE_MDF_INNER_PROD
static spx_word32_t mdf_inner_prod(const spx_word16_t * x, const spx_word16_t * y, int len)
{
    spx_word32_t    sum = 0;
    len >>= 1;
    while (len--) {
        spx_word32_t    part = 0;
        part = MAC16_16(part, *x++, *y++);
        part = MAC16_16(part, *x++, *y++);
        /* HINT: If you had a 40-bit accumulator, you could shift only at the end */
        sum = ADD32(sum, SHR32(part, 6));
    }
    return sum;
}
#endif

#ifdef OVERRIDE_MDF_POWER_SPECTRUM
static void power_spectrum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    int             i, j;
    ps[0] = MULT16_16(X[0], X[0]);
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        ps[j] = MULT16_16(X[i], X[i]) + MULT16_16(X[i + 1], X[i + 1]);
    }
    ps[j] = MULT16_16(X[i], X[i]);
}
#endif

/** Compute power spectrum of a half-complex (packed) vector and accumulate */
#ifdef OVERRIDE_MDF_POWER_SPECTRUM_ACCUM
static void power_spectrum_accum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    int             i, j;
    ps[0] += MULT16_16(X[0], X[0]);
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        ps[j] += MULT16_16(X[i], X[i]) + MULT16_16(X[i + 1], X[i + 1]);
    }
    ps[j] += MULT16_16(X[i], X[i]);
}
#endif


#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM
#ifndef FIXED_POINT
static void spectral_mul_accum(const spx_word16_t * X, const spx_word32_t * Y, spx_word16_t * acc, int N, int M)
{
    int             i, j;
    for (i = 0; i < N; i++)
        acc[i] = 0;
    for (j = 0; j < M; j++) {
        acc[0] += X[0] * Y[0];
        for (i = 1; i < N - 1; i += 2) {
            acc[i] += (X[i] * Y[i] - X[i + 1] * Y[i + 1]);
            acc[i + 1] += (X[i + 1] * Y[i] + X[i] * Y[i + 1]);
        }
        acc[i] += X[i] * Y[i];
        X += N;
        Y += N;
    }
}
#else
static inline void spectral_mul_accum(const spx_word16_t *X, const spx_word32_t *Y, spx_word16_t *acc, int N, int M)
{
   int i,j;
   spx_word32_t tmp1=0,tmp2=0;
   for (j=0;j<M;j++)
   {
      tmp1 = MAC16_16(tmp1, X[j*N],TOP16(Y[j*N]));
   }
   acc[0] = PSHR32(tmp1,WEIGHT_SHIFT);
   for (i=1;i<N-1;i+=2)
   {
      tmp1 = tmp2 = 0;
      for (j=0;j<M;j++)
      {
         tmp1 = SUB32(MAC16_16(tmp1, X[j*N+i],TOP16(Y[j*N+i])), MULT16_16(X[j*N+i+1],TOP16(Y[j*N+i+1])));
         tmp2 = MAC16_16(MAC16_16(tmp2, X[j*N+i+1],TOP16(Y[j*N+i])), X[j*N+i], TOP16(Y[j*N+i+1]));
      }
      acc[i] = PSHR32(tmp1,WEIGHT_SHIFT);
      acc[i+1] = PSHR32(tmp2,WEIGHT_SHIFT);
   }
   tmp1 = tmp2 = 0;
   for (j=0;j<M;j++)
   {
      tmp1 = MAC16_16(tmp1, X[(j+1)*N-1],TOP16(Y[(j+1)*N-1]));
   }
   acc[N-1] = PSHR32(tmp1,WEIGHT_SHIFT);
}
#endif

#endif

#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM16
static inline void spectral_mul_accum16(const spx_word16_t *X, const spx_word16_t *Y, spx_word16_t *acc, int N, int M)
{
   int i,j;
   spx_word32_t tmp1=0,tmp2=0;
   for (j=0;j<M;j++)
   {
      tmp1 = MAC16_16(tmp1, X[j*N],Y[j*N]);
   }
   acc[0] = PSHR32(tmp1,WEIGHT_SHIFT);
   for (i=1;i<N-1;i+=2)
   {
      tmp1 = tmp2 = 0;
      for (j=0;j<M;j++)
      {
         tmp1 = SUB32(MAC16_16(tmp1, X[j*N+i],Y[j*N+i]), MULT16_16(X[j*N+i+1],Y[j*N+i+1]));
         tmp2 = MAC16_16(MAC16_16(tmp2, X[j*N+i+1],Y[j*N+i]), X[j*N+i], Y[j*N+i+1]);
      }
      acc[i] = PSHR32(tmp1,WEIGHT_SHIFT);
      acc[i+1] = PSHR32(tmp2,WEIGHT_SHIFT);
   }
   tmp1 = tmp2 = 0;
   for (j=0;j<M;j++)
   {
      tmp1 = MAC16_16(tmp1, X[(j+1)*N-1],Y[(j+1)*N-1]);
   }
   acc[N-1] = PSHR32(tmp1,WEIGHT_SHIFT);
}
#endif

#ifdef OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ
static void weighted_spectral_mul_conj(const spx_float_t * w, const spx_float_t p, const spx_word16_t * X, const spx_word16_t * Y, spx_word32_t * prod, int N)
{
    int             i, j;
    spx_float_t     W;
    W = FLOAT_AMULT(p, w[0]);
    prod[0] = FLOAT_MUL32(W, MULT16_16(X[0], Y[0]));
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        W = FLOAT_AMULT(p, w[j]);
        prod[i] = FLOAT_MUL32(W, MAC16_16(MULT16_16(X[i], Y[i]), X[i + 1], Y[i + 1]));
        prod[i + 1] = FLOAT_MUL32(W, MAC16_16(MULT16_16(-X[i + 1], Y[i]), X[i], Y[i + 1]));
    }
    W = FLOAT_AMULT(p, w[j]);
    prod[i] = FLOAT_MUL32(W, MULT16_16(X[i], Y[i]));
}
#endif

#ifdef OVERRIDE_MDF_ADJUST_PROP
static void mdf_adjust_prop(const spx_word32_t * W, int N, int M, int P, spx_word16_t * prop)
{
    int             i, j, p;
    spx_word16_t    max_sum = 1;
    spx_word32_t    prop_sum = 1;
    for (i = 0; i < M; i++) {
        spx_word32_t    tmp = 1;
        for (p = 0; p < P; p++)
            for (j = 0; j < N; j++)
                tmp += MULT16_16(EXTRACT16(SHR32(W[p * N * M + i * N + j], 18)), EXTRACT16(SHR32(W[p * N * M + i * N + j], 18)));
#ifdef FIXED_POINT
        /* Just a security in case an overflow were to occur */
        tmp = MIN32(ABS32(tmp), 536870912);
#endif
        prop[i] = spx_sqrt(tmp);
        if (prop[i] > max_sum)
            max_sum = prop[i];
    }
    for (i = 0; i < M; i++) {
        prop[i] += MULT16_16_Q15(QCONST16(.1f, 15), max_sum);
        prop_sum += EXTEND32(prop[i]);
    }
    for (i = 0; i < M; i++) {
        prop[i] = DIV32(MULT16_16(QCONST16(.99f, 15), prop[i]), prop_sum);
        /*printf ("%f ", prop[i]); */
    }
    /*printf ("\n"); */
}

#endif

#ifdef OVERRIDE_MDF_PREEMPH_FLT
static int mdf_preemph(spx_word16_t * in, spx_word16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem)
{
    int             i;
    int             saturated = 0;

    for (i = 0; i < len; i++) {
        spx_word32_t    tmp32;
        /* FIXME: This core has changed a bit, need to merge properly */
        tmp32 = SUB32(EXTEND32(in[i]), EXTEND32(MULT16_16_P15(preemph, *mem)));
#ifdef FIXED_POINT
        if (tmp32 > 32767) {
            tmp32 = 32767;
            if (saturated == 0)
                saturated = 1;
        }
        if (tmp32 < -32767) {
            tmp32 = -32767;
            if (saturated == 0)
                saturated = 1;
        }
#endif
        *mem = in[i];
        out[i] = EXTRACT16(tmp32);
    }
    return saturated;
}
#endif


#ifdef OVERRIDE_MDF_STRIDED_PREEMPH_FLT
static int mdf_preemph_with_stride_int(const spx_int16_t * in, spx_word16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem, int stride)
{
    int             i;
    int             saturated = 0;

    for (i = 0; i < len; i++) {
        spx_word32_t    tmp32;
        tmp32 = SUB32(EXTEND32(in[i * stride]), EXTEND32(MULT16_16_P15(preemph, *mem)));
#ifdef FIXED_POINT
        /*FIXME: If saturation occurs here, we need to freeze adaptation for M frames (not just one) */
        if (tmp32 > 32767) {
            tmp32 = 32767;
            saturated = 1;
        }
        if (tmp32 < -32767) {
            tmp32 = -32767;
            saturated = 1;
        }
#endif
        out[i] = EXTRACT16(tmp32);
        *mem = in[i * stride];
    }
    return saturated;
}
#endif


#ifdef OVERRIDE_MDF_VEC_SUB
static void vect_sub(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = SUB16(pSrcA[i], pSrcB[i]);
}
#endif


#ifdef OVERRIDE_MDF_VEC_SUB16
static void vect_sub16(const spx_int16_t * pSrcA, const spx_int16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = (pSrcA[i] - pSrcB[i]);
}
#endif


#ifdef OVERRIDE_MDF_VEC_ADD
static void vect_add(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = pSrcA[i] + pSrcB[i];
}
#endif


#ifdef OVERRIDE_MDF_VEC_MULT
/* vector mult for windowing */
static void vect_mult(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = MULT16_16_Q15(pSrcA[i], pSrcB[i]);
}
#endif

#ifdef OVERRIDE_MDF_VEC_SCALE
static void vect_scale(const spx_word16_t * pSrc, spx_word16_t scale, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = (spx_int32_t) MULT16_32_Q15(scale, pSrc[i]);
}
#endif

#ifdef OVERRIDE_MDF_VEC_CLEAR
static void vect_clear(spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = 0.0f;
}
#endif

#ifdef OVERRIDE_MDF_VEC_COPY
static void vect_copy(void *dst, const void *src, uint32_t blockSize)
{
    th_memcpy(dst, src, blockSize);
}
#endif


#ifdef OVERRIDE_MDF_SMOOTHED_ADD
static void smoothed_add(const spx_word16_t * pSrc1, const spx_word16_t * pWin1,
                         const spx_word16_t * pSrc2, const spx_word16_t * pWin2, spx_word16_t * pDst, uint16_t frame_size, uint16_t nbChan, uint16_t N)
{
    int             chan;
    int             i;

    for (chan = 0; chan < nbChan; chan++)
        for (i = 0; i < frame_size; i++)
            pDst[chan * N + i] = MULT16_16_Q15(pWin1[i], pSrc1[chan * N + i])
                + MULT16_16_Q15(pWin2[i], pSrc2[chan * N + i]);

}
#endif


#ifdef OVERRIDE_MDF_DEEMPH
static int mdf_deemph(const spx_int16_t * micin, spx_word16_t * input,
                      spx_word16_t * e, spx_int16_t * out, spx_word16_t preemph, int frame_size, spx_word16_t * mem, int stride)
{
    int32_t         saturated = 0;
    int             i;

    for (i = 0; i < frame_size; i++) {
        spx_word32_t    tmp_out;
#ifdef TWO_PATH
        tmp_out = SUB32(EXTEND32(input[i]), EXTEND32(e[i]));
#else
        tmp_out = SUB32(EXTEND32(input[i]), EXTEND32(y[i]));
#endif
        tmp_out = ADD32(tmp_out, EXTEND32(MULT16_16_P15(preemph, *mem)));
        /* This is an arbitrary test for saturation in the microphone signal */
        if (micin[i * stride] <= -32000 || micin[i * stride] >= 32000) {
            if (saturated == 0)
                saturated = 1;
        }
        out[i * stride] = WORD2INT(tmp_out);
        *mem = tmp_out;
    }
    return saturated;
}
#endif

#ifdef OVERRIDE_MDF_SMOOTH_FE_NRG
static void smooth_fe_nrg(spx_word32_t * in1, spx_word16_t c1, spx_word32_t * in2, spx_word16_t c2, spx_word32_t * pDst, uint16_t frame_size)
{
    int             j;

    for (j = 0; j <= frame_size; j++)
        pDst[j] = MULT16_32_Q15(c1, in1[j]) + Q0CONST(1) + MULT16_32_Q15(c2, in2[j]);
}
#endif

#ifdef OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR
static void filtered_spectra_cross_corr(spx_word32_t * pRf, spx_word32_t * pEh, spx_word32_t * pYf, spx_word32_t * pYh,
                                        spx_float_t * Pey, spx_float_t * Pyy, spx_word16_t spec_average, uint16_t frame_size)
{
    int             j;

    for (j = frame_size; j >= 0; j--) {
        spx_float_t     Eh, Yh;
        Eh = PSEUDOFLOAT(pRf[j] - pEh[j]);
        Yh = PSEUDOFLOAT(pYf[j] - pYh[j]);
        *Pey = FLOAT_ADD(*Pey, FLOAT_MULT(Eh, Yh));
        *Pyy = FLOAT_ADD(*Pyy, FLOAT_MULT(Yh, Yh));
#ifdef FIXED_POINT
        pEh[j] = MAC16_32_Q15(MULT16_32_Q15(SUB16(32767, spec_average), pEh[j]), spec_average, pRf[j]);
        pYh[j] = MAC16_32_Q15(MULT16_32_Q15(SUB16(32767, spec_average), pYh[j]), spec_average, pYf[j]);
#else
        pEh[j] = (1 - spec_average) * pEh[j] + spec_average * pRf[j];
        pYh[j] = (1 - spec_average) * pYh[j] + spec_average * pYf[j];
#endif
    }
}
#endif


#ifdef OVERRIDE_MDF_NORM_LEARN_RATE_CALC
static void mdf_nominal_learning_rate_calc(spx_word32_t * pRf, spx_word32_t * power,
                                           spx_word32_t * pYf, spx_float_t * power_1, spx_word16_t leak_estimate, spx_word16_t RER, uint16_t frame_size)
{
    int             i;

    for (i = 0; i < frame_size; i++) {
        spx_word32_t    r, e;
        /* Compute frequency-domain adaptation mask */
        r = MULT16_32_Q15(leak_estimate, SHL32(pYf[i], 3));
        e = SHL32(pRf[i], 3) + 1;
#ifdef FIXED_POINT
        if (r > SHR32(e, 1))
            r = SHR32(e, 1);
#else
        if (r > .5f * e)
            r = .5f * e;
#endif
        r = MULT16_32_Q15(QCONST16(.7f, 15), r) + MULT16_32_Q15(QCONST16(.3f, 15), (spx_word32_t) (MULT16_32_Q15(RER, e)));
        /*st->power_1[i] = adapt_rate*r/(e*(1+st->power[i])); */
        power_1[i] = FLOAT_SHL(FLOAT_DIV32_FLOAT(r, FLOAT_MUL32U(e, power[i] + 10.f)), WEIGHT_SHIFT + 16);
    }
}

#endif

#ifdef OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC
static void mdf_non_adapt_learning_rate_calc(spx_word32_t * power, spx_float_t * power_1, spx_word16_t adapt_rate, uint16_t frame_size)
{
    int             i;

    for (i = 0; i < frame_size; i++)
        power_1[i] = FLOAT_SHL(FLOAT_DIV32(EXTEND32(adapt_rate), ADD32(power[i], Q0CONST(10))), WEIGHT_SHIFT + 1);
}

#endif
