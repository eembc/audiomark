/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_libspeex_kernels.c
 * Description:  optimized kernels for LIBSPEEX
 *
 * $Date:        23 April 2021
 * $Revision:    V1.9.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#if defined(USE_CMSIS_DSP)

    // 
    // List of kernels 
    // 
    // arm_filter_dc_notch16
    // arm_mdf_inner_prod
    // arm_power_spectrum
    // arm_power_spectrum_accum
    // arm_spectral_mul_accum
    // arm_spectral_mul_accum16
    // arm_weighted_spectral_mul_conj
    // arm_mdf_adjust_prop
    //  

#include "arch.h"
#include "speex/speex_echo.h"
#include "fftwrap.h"
#include "pseudofloat.h"
#include "math_approx.h"
#include "os_support.h"

#include "kiss_fftr.h"
#include "kiss_fft.h"



void arm_filter_dc_notch16(const spx_int16_t *in, spx_word16_t radius, spx_word16_t *out, int len, spx_mem_t *mem, int stride)
{
   int i;
   spx_word16_t den2;
#ifdef FIXED_POINT
   den2 = MULT16_16_Q15(radius,radius) + MULT16_16_Q15(QCONST16(.7,15),MULT16_16_Q15(32767-radius,32767-radius));
#else
   den2 = radius*radius + .7*(1-radius)*(1-radius);
#endif
   /*printf ("%d %d %d %d %d %d\n", num[0], num[1], num[2], den[0], den[1], den[2]);*/
   for (i=0;i<len;i++)
   {
      spx_word16_t vin = in[i*stride];
      spx_word32_t vout = mem[0] + SHL32(EXTEND32(vin),15);
#ifdef FIXED_POINT
      mem[0] = mem[1] + SHL32(SHL32(-EXTEND32(vin),15) + MULT16_32_Q15(radius,vout),1);
#else
      mem[0] = mem[1] + 2*(-vin + radius*vout);
#endif
      mem[1] = SHL32(EXTEND32(vin),15) - MULT16_32_Q15(den2,vout);
      out[i] = SATURATE32(PSHR32(MULT16_32_Q15(radius,vout),15),32767);
   }
}

/* This inner product is slightly different from the codec version because of fixed-point */
spx_word32_t arm_mdf_inner_prod(const spx_word16_t *x, const spx_word16_t *y, int len)
{
   spx_word32_t sum=0;
   len >>= 1;
   while(len--)
   {
      spx_word32_t part=0;
      part = MAC16_16(part,*x++,*y++);
      part = MAC16_16(part,*x++,*y++);
      /* HINT: If you had a 40-bit accumulator, you could shift only at the end */
      sum = ADD32(sum,SHR32(part,6));
   }
   return sum;
}

/** Compute power spectrum of a half-complex (packed) vector */
void arm_power_spectrum(const spx_word16_t *X, spx_word32_t *ps, int N)
{
   int i, j;
   ps[0]=MULT16_16(X[0],X[0]);
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      ps[j] =  MULT16_16(X[i],X[i]) + MULT16_16(X[i+1],X[i+1]);
   }
   ps[j]=MULT16_16(X[i],X[i]);
}

/** Compute power spectrum of a half-complex (packed) vector and accumulate */
void arm_power_spectrum_accum(const spx_word16_t *X, spx_word32_t *ps, int N)
{
   int i, j;
   ps[0]+=MULT16_16(X[0],X[0]);
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      ps[j] +=  MULT16_16(X[i],X[i]) + MULT16_16(X[i+1],X[i+1]);
   }
   ps[j]+=MULT16_16(X[i],X[i]);
}

void arm_spectral_mul_accum(const spx_word16_t *X, const spx_word32_t *Y, spx_word16_t *acc, int N, int M)
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
void arm_spectral_mul_accum16(const spx_word16_t *X, const spx_word16_t *Y, spx_word16_t *acc, int N, int M)
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

void arm_weighted_spectral_mul_conj(const spx_float_t *w, const spx_float_t p, const spx_word16_t *X, const spx_word16_t *Y, spx_word32_t *prod, int N)
{
   int i, j;
   spx_float_t W;
   W = FLOAT_AMULT(p, w[0]);
   prod[0] = FLOAT_MUL32(W,MULT16_16(X[0],Y[0]));
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      W = FLOAT_AMULT(p, w[j]);
      prod[i] = FLOAT_MUL32(W,MAC16_16(MULT16_16(X[i],Y[i]), X[i+1],Y[i+1]));
      prod[i+1] = FLOAT_MUL32(W,MAC16_16(MULT16_16(-X[i+1],Y[i]), X[i],Y[i+1]));
   }
   W = FLOAT_AMULT(p, w[j]);
   prod[i] = FLOAT_MUL32(W,MULT16_16(X[i],Y[i]));
}



void arm_mdf_adjust_prop(const spx_word32_t *W, int N, int M, int P, spx_word16_t *prop)
{
   int i, j, p;
   spx_word16_t max_sum = 1;
   spx_word32_t prop_sum = 1;
   for (i=0;i<M;i++)
   {
      spx_word32_t tmp = 1;
      for (p=0;p<P;p++)
         for (j=0;j<N;j++)
            tmp += MULT16_16(EXTRACT16(SHR32(W[p*N*M + i*N+j],18)), EXTRACT16(SHR32(W[p*N*M + i*N+j],18)));
#ifdef FIXED_POINT
      /* Just a security in case an overflow were to occur */
      tmp = MIN32(ABS32(tmp), 536870912);
#endif
      prop[i] = spx_sqrt(tmp);
      if (prop[i] > max_sum)
         max_sum = prop[i];
   }
   for (i=0;i<M;i++)
   {
      prop[i] += MULT16_16_Q15(QCONST16(.1f,15),max_sum);
      prop_sum += EXTEND32(prop[i]);
   }
   for (i=0;i<M;i++)
   {
      prop[i] = DIV32(MULT16_16(QCONST16(.99f,15), prop[i]),prop_sum);
      /*printf ("%f ", prop[i]);*/
   }
   /*printf ("\n");*/
}


static int maximize_range(spx_word16_t *in, spx_word16_t *out, spx_word16_t bound, int len)
{
   int i, shift;
   spx_word16_t max_val = 0;
   for (i=0;i<len;i++)
   {
      if (in[i]>max_val)
         max_val = in[i];
      if (-in[i]>max_val)
         max_val = -in[i];
   }
   shift=0;
   while (max_val <= (bound>>1) && max_val != 0)
   {
      max_val <<= 1;
      shift++;
   }
   for (i=0;i<len;i++)
   {
      out[i] = SHL16(in[i], shift);
   }
   return shift;
}

static void renorm_range(spx_word16_t *in, spx_word16_t *out, int shift, int len)
{
   int i;
   for (i=0;i<len;i++)
   {
      out[i] = PSHR16(in[i], shift);
   }
}

struct kiss_config {
   kiss_fftr_cfg forward;
   kiss_fftr_cfg backward;
   int N;
};

void *arm_spx_fft_init(int size)
{
   struct kiss_config *table;
   table = (struct kiss_config*)speex_alloc(sizeof(struct kiss_config));
   table->forward = kiss_fftr_alloc(size,0,NULL,NULL);
   table->backward = kiss_fftr_alloc(size,1,NULL,NULL);
   table->N = size;
   return table;
}

void arm_spx_fft_destroy(void *table)
{
   struct kiss_config *t = (struct kiss_config *)table;
   kiss_fftr_free(t->forward);
   kiss_fftr_free(t->backward);
   speex_free(table);
}

void arm_spx_fft(void *table, spx_word16_t *in, spx_word16_t *out)
{
   int shift;
   struct kiss_config *t = (struct kiss_config *)table;
   shift = maximize_range(in, in, 32000, t->N);
   kiss_fftr2(t->forward, in, out);
   renorm_range(in, in, shift, t->N);
   renorm_range(out, out, shift, t->N);
}

void arm_spx_ifft(void *table, spx_word16_t *in, spx_word16_t *out)
{
   struct kiss_config *t = (struct kiss_config *)table;
   kiss_fftri2(t->backward, in, out);
}

#endif  // #if defined(USE_CMSIS_DSP)