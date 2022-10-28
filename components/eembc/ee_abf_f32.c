#include "ee_abf_f32.h"

extern const float w_hanning_div2[];
extern const float rotation[];

ee_abf_f32_params_t bf_prms;
ee_abf_f32_mem_t    bf_mem;

beamformer_f32_instance bf_instance;

/* Fast coefficient structure of the instance */
ee_abf_f32_static_t  beamformer_f32_fastdata_static;
ee_abf_f32_working_t beamformer_f32_fastdata_working;

static void
ee_abf_f32_init(ee_abf_f32_params_t *bf_prms, ee_abf_f32_mem_t *bf_mem)
{
    bf_prms->alpha_BM_NLMS = 0.01f;
    bf_prms->DS_DET_TH     = 0.2f;
    bf_prms->ep_GSC        = 1e-12f;

    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        for (int j = 0; j < LEN_BM_ADF * 2; j++)
        {
            bf_mem->states_BM_ADF[i][j] = 0;
            bf_mem->coefs_BM_ADF[i][j]  = 0;
        }
        bf_mem->Norm_out_BM[i] = 0;
        bf_mem->lookBF_out[i]  = 0;
    }
    bf_mem->GSC_det_avg                = 0;
    bf_mem->adptBF_coefs_update_enable = 0;
}

static void
adaptive_beamformer(ee_f32_t           *bf_cmplx_in_pt,
                    ee_f32_t           *bm_cmplx_in_pt,
                    ee_f32_t           *adap_cmplx_out_pt,
                    ee_abf_f32_params_t bf_prms,
                    ee_abf_f32_mem_t   *bf_mem)
{
    ee_f32_t adap_out[2], error_out[2];
    ee_f32_t temp[LEN_BM_ADF * 2];
    ee_f32_t sum0 = 0.0f;
    ee_f32_t sum1 = 0.0f;

    // Update delay line for reference signal
    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        for (int j = (LEN_BM_ADF * 2 - 3); j >= 0; j--)
        {
            bf_mem->states_BM_ADF[i][j + 2] = bf_mem->states_BM_ADF[i][j];
        }
    }
    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        bf_mem->states_BM_ADF[i][0] = bm_cmplx_in_pt[2 * i];
        bf_mem->states_BM_ADF[i][1] = bm_cmplx_in_pt[2 * i + 1];
    }

    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        // adaptive filter
        th_cmplx_conj_f32(&bf_mem->coefs_BM_ADF[i][0], temp, LEN_BM_ADF);
        th_cmplx_dot_prod_f32(temp,
                              &bf_mem->states_BM_ADF[i][0],
                              LEN_BM_ADF,
                              &adap_out[0],
                              &adap_out[1]);
        // calculate error
        error_out[0] = bf_cmplx_in_pt[2 * i] - adap_out[0];
        error_out[1] = bf_cmplx_in_pt[2 * i + 1] - adap_out[1];
        if (bf_mem->adptBF_coefs_update_enable
            && bf_mem->GSC_det_avg > bf_prms.DS_DET_TH)
        { // update adptBF coefficients
            float tmp = bf_prms.alpha_BM_NLMS
                        / (bf_mem->Norm_out_BM[i] + bf_prms.ep_GSC);
            for (int j = 0; j < LEN_BM_ADF * 2; j += 2)
            {
                bf_mem->coefs_BM_ADF[i][j]
                    += tmp
                       * (error_out[0] * bf_mem->states_BM_ADF[i][j]
                          + error_out[1] * bf_mem->states_BM_ADF[i][j + 1]);
                bf_mem->coefs_BM_ADF[i][j + 1]
                    += tmp
                       * (error_out[0] * bf_mem->states_BM_ADF[i][j + 1]
                          - error_out[1] * bf_mem->states_BM_ADF[i][j]);
            }
        }
        adap_cmplx_out_pt[2 * i]     = error_out[0];
        adap_cmplx_out_pt[2 * i + 1] = error_out[1];
        bf_mem->Norm_out_BM[i]
            = 0.9f * bf_mem->Norm_out_BM[i]
              + 0.1f
                    * (bm_cmplx_in_pt[2 * i] * bm_cmplx_in_pt[2 * i]
                       + bm_cmplx_in_pt[2 * i + 1] * bm_cmplx_in_pt[2 * i + 1]);
        sum0 += bf_mem->Norm_out_BM[i];
        bf_mem->lookBF_out[i]
            = 0.9f * bf_mem->lookBF_out[i]
              + 0.1f
                    * (bf_cmplx_in_pt[2 * i] * bf_cmplx_in_pt[2 * i]
                       + bf_cmplx_in_pt[2 * i + 1] * bf_cmplx_in_pt[2 * i + 1]);
        sum1 += bf_mem->lookBF_out[i];
    }
    // update GSC_det_avg
    bf_mem->GSC_det_avg
        = 0.9f * bf_mem->GSC_det_avg + 0.1f * sum0 / (sum1 + bf_prms.ep_GSC);
    if (bf_mem->GSC_det_avg > 2.0f)
        bf_mem->GSC_det_avg = 2.0f;
    // generate output
    for (int i = 0; i < (NFFT / 2) - 1; i++)
    {
        adap_cmplx_out_pt[2 * i + NFFT + 2]
            = adap_cmplx_out_pt[NFFT - 2 * i - 2];
        adap_cmplx_out_pt[1 + 2 * i + NFFT + 2]
            = -adap_cmplx_out_pt[NFFT - 2 * i - 1];
    }
}

static void
ee_abf_f32_run(beamformer_f32_instance *instance,
               int16_t                 *input_buffer_left,
               int16_t                 *input_buffer_right,
               int32_t                  input_buffer_size,
               int16_t                 *output_buffer,
               int32_t                 *input_samples_consumed,
               int32_t                 *output_samples_produced,
               int32_t                 *returned_state)
{
    int32_t input_index;
    int32_t ilag;
    int32_t i;
    float  *pf32_1;
    float  *pf32_2;
    float  *pf32_3;
    float  *pf32_out;
    float   ftmp;

    beamformer_f32_instance *pinstance = (beamformer_f32_instance *)instance;

    input_index = 0;
    while (input_index + NFFTD2 <= input_buffer_size)
    {
        /* datachunkLeft = [old_left ; inputLeft]; old_left = inputLeft;
           datachunkRight = [old_right ; inputRight]; old_right = inputRight;
           X0 = fft(datachunkLeft );
           Y0 = fft(datachunkRight);
        */
        pf32_1 = pinstance->st->old_left;
        pf32_2 = pinstance->w->X0; /* old samples -> mic[0..NFFTD2[ */
        for (i = 0; i < NFFTD2 * COMPLEX; i++)
        {
            *pf32_2++ = *pf32_1++;
        }
        pf32_3 = pinstance->w->CY0; /* temporary buffer */
        th_int16_to_f32(&(input_buffer_left[input_index]), pf32_3, NFFTD2);

        pf32_1 = pinstance->st->old_left; /* save samples for next frame */
        for (i = 0; i < NFFTD2 * REAL; i++)
        {
            *pf32_1++ = *pf32_2++ = *pf32_3++;
            *pf32_1++ = *pf32_2++ = 0;
        }
        pf32_2 = pinstance->w->X0;
        th_cfft_f32(&((pinstance->st)->cS), pf32_2, 0, 1);

        /* Right microphone */
        pf32_1 = pinstance->st->old_right;
        pf32_2 = pinstance->w->Y0; /* old samples -> mic[0..NFFTD2[ */
        for (i = 0; i < NFFTD2 * COMPLEX; i++)
        {
            *pf32_2++ = *pf32_1++;
        }
        pf32_3 = pinstance->w->CY0; /* temporary buffer */
        th_int16_to_f32(&(input_buffer_right[input_index]), pf32_3, NFFTD2);

        pf32_1 = pinstance->st->old_right; /* save samples for next frame */
        for (i = 0; i < NFFTD2 * REAL; i++)
        {
            *pf32_1++ = *pf32_2++ = *pf32_3++;
            *pf32_1++ = *pf32_2++ = 0;
        }
        pf32_2 = pinstance->w->Y0;
        th_cfft_f32(&((pinstance->st)->cS), pf32_2, 0, 1);

        /* XY = X0(HalfRange) .* conj(Y0(HalfRange));
         */
        pf32_1 = pinstance->w->Y0;
        pf32_2 = pinstance->w->CY0;
        th_cmplx_conj_f32(pf32_1, pf32_2, NFFTD2);

        pf32_1   = pinstance->w->X0;
        pf32_2   = pinstance->w->CY0;
        pf32_out = pinstance->w->XY;
        th_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_out, NFFTD2);

        /* PHATNORM = max(abs(XY), 1e-12);
         */
        pf32_1   = pinstance->w->XY;
        pf32_out = pinstance->w->PHATNORM;
        th_cmplx_mag_f32(pf32_1, pf32_out, NFFTD2);

        /*  XY = XY ./ PHATNORM;
         */
        pf32_1 = pinstance->w->PHATNORM;
        pf32_2 = pinstance->w->XY;
        for (i = 0; i < NFFT; i++)
        {
            ftmp = *pf32_1++;
            if (ftmp == 0)
                continue;
            *pf32_2++ /= ftmp; // real part
            *pf32_2++ /= ftmp; // imaginary part
        }

        /* for ilag = LagRange         % check all angles
            ZZ = XY .* wrot(idxLag,HalfRange).';
            allDerot(idxLag) = sum(real(ZZ));
            idxLag = idxLag+1;      % save all the beams
        end
        [corr, icorr] = max(allDerot); % keep the best one
        */
        pf32_1   = pinstance->wrot;
        pf32_2   = pinstance->w->XY;
        pf32_out = pinstance->w->allDerot;
        for (ilag = 0; ilag < LAGSTEP; ilag++)
        {
            pf32_3 = pinstance->w->CY0; /* ZZ in Matlab reference */
            th_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_3, NFFTD2);
            ftmp = 0;
            for (i = 0; i < NFFTD2; i++)
            {
                ftmp += (*pf32_3);
                pf32_3 += 2; /* sum of the real */
            }
            *pf32_out++ = ftmp;
            pf32_1 += NFFT * COMPLEX; /* next rotation vector */
        }
        pf32_out = pinstance->w->allDerot;
        th_absmax_f32(
            pf32_out, NFFTD2, &(pinstance->w->corr), &(pinstance->w->icorr));

        /* SYNTHESIS
           wrot2 = wrot(fixed_lag,:);
           NewSpectrum = X0 + Y0.*wrot2.';
        */
        pf32_1   = pinstance->wrot + (FIXED_DIRECTION * NFFT);
        pf32_2   = pinstance->w->Y0;
        pf32_out = pinstance->w->XY; /* temporary buffer Y0.*wrot2 */
        th_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_out, NFFT);

        pf32_1   = pinstance->w->X0;
        pf32_2   = pinstance->w->XY;
        pf32_out = pinstance->w->BF; /* (X0 + Y0.*wrot2.') = fix_bf_out() */
        th_add_f32(pf32_1, pf32_2, pf32_out, NFFT * COMPLEX);
        pf32_out = pinstance->w->BM; /* (X0 - Y0.*wrot2.') = fix_bm_out() */
        th_subtract_f32(pf32_1, pf32_2, pf32_out, NFFT * COMPLEX);

        /* Synthesis = 0.5*hann(NFFT) .* real(ifft(NewSpectrum));
           Synthesis_adap = w_hann .* real(ifft(NewSpectrum_adap));

          with NewSpectrum_adap = adaptive_beamformer(BF, BM, out, states, mem);
        */
        pf32_1   = pinstance->w->BF;
        pf32_2   = pinstance->w->BM;
        pf32_out = pinstance->w->CY0;
        adaptive_beamformer(pf32_1,
                            pf32_2,
                            pf32_out,
                            bf_prms,
                            &bf_mem); /* CY0 = synthesis spectrum */

        pf32_2 = pinstance->w->CY0;
        th_cfft_f32(
            &((pinstance->st)->cS), pf32_2, 1, 1); /* in-place processing */

        pf32_1 = pf32_2;
        for (i = 0; i < NFFT; i++) /* extract the real part */
        {
            *pf32_1++ = *pf32_2;
            pf32_2 += COMPLEX;
        }

        pf32_1 = pinstance->w->CY0; /* apply the Hanning window */
        pf32_2 = pinstance->window;
        pf32_3 = pinstance->w->CY0; /* hanning window temporary */
        th_multiply_f32(pf32_1, pf32_2, pf32_3, NFFTD2);

        pf32_1 = pinstance->st
                     ->ola_new; /* overlap and add with the previous buffer */
        pf32_2 = pinstance->w->CY0;
        th_add_f32(pf32_1, pf32_2, pf32_2, NFFTD2);
        th_f32_to_int16(pf32_2, output_buffer, NFFTD2);

        pf32_1 = pinstance->w->CY0 + NFFTD2;
        pf32_2 = pinstance->window + NFFTD2;
        pf32_3 = pinstance->st->ola_new;
        th_multiply_f32(pf32_1, pf32_2, pf32_3, NFFTD2);

        input_index += NFFTD2; /* number of samples used in the input buffer */
        output_buffer += NFFTD2;
    }

    *input_samples_consumed  = input_index;
    *output_samples_produced = input_index;
    *returned_state          = 0;
}

void
th_beamformer_f32_reset(void)
{
    int i;

    bf_instance.window = (float *)w_hanning_div2;
    bf_instance.wrot   = (float *)rotation;
    bf_instance.st     = &beamformer_f32_fastdata_static;
    bf_instance.w      = &beamformer_f32_fastdata_working;

    /* reset static buffers */
    for (i = 0; i < NFFT; i++)
    {
        bf_instance.st->old_left[i]  = 0;
        bf_instance.st->old_right[i] = 0;
    }
    for (i = 0; i < NFFTD2; i++)
    {
        bf_instance.st->ola_new[i] = 0;
    }
    /* init rFFT tables */
    th_rfft_init_f32(&((bf_instance.st)->rS), NFFT);
    /* init cFFT tables */
    th_cfft_init_f32(&((bf_instance.st)->cS), NFFT);
    /* adaptive filter reset */
    ee_abf_f32_init(&bf_prms, &bf_mem);
    bf_mem.GSC_det_avg                = 0; // 0.94083f;
    bf_mem.adptBF_coefs_update_enable = 1;
}

int32_t
ee_abf_f32(int32_t command, void **instance, void *data, void *parameters)
{
    int32_t swc_returned_status = 0;

    switch (command)
    {
        case NODE_MEMREQ:
            *(uint32_t *)(*instance) = 0;
            break;
        case NODE_RESET:
            th_beamformer_f32_reset();
            break;
        case NODE_RUN: {
            PTR_INT *pt_pt = NULL;
            uint32_t buffer1_size;
            uint32_t buffer2_size;
            int32_t  nb_input_samples;
            int32_t  input_samples_consumed;
            int32_t  output_samples_produced;
            int16_t *inBufs1stChannel = NULL;
            int16_t *inBufs2ndChannel = NULL;
            int16_t *outBufs          = NULL;

            pt_pt            = (PTR_INT *)data;
            inBufs1stChannel = (int16_t *)(*pt_pt++);
            buffer1_size     = (uint32_t)(*pt_pt++);
            inBufs2ndChannel = (int16_t *)(*pt_pt++);
            buffer2_size     = (uint32_t)(*pt_pt++);
            outBufs          = (int16_t *)(*pt_pt++);

            nb_input_samples = buffer1_size / sizeof(int16_t);
            if (buffer2_size != buffer1_size)
            {
                return 1;
            }
            ee_abf_f32_run(&bf_instance,
                           (int16_t *)inBufs1stChannel,
                           (int16_t *)inBufs2ndChannel,
                           nb_input_samples,
                           (int16_t *)outBufs,
                           &input_samples_consumed,
                           &output_samples_produced,
                           &swc_returned_status);
            break;
        }
    }
    return swc_returned_status;
}
