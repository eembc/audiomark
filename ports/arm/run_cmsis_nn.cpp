/**
 * Copyright (C) 2023 EEMBC
 * Copyright (C) 2024 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 */

#define restrict __restrict__

/* temporary GCC + MVE workaround */
#if defined(__ARM_FEATURE_MVE) && __ARM_FEATURE_MVE
#include <arm_mve.h>
#endif

extern "C" {

#include "ee_audiomark.h"
#include "ee_api.h"
#include "ee_mfcc_f32.h"
#include "ee_nn.h"
}

#include "include/BufAttributes.hpp" /* Buffer attributes to be applied */
#include "include/ds_cnn_model.hpp"

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */
#include "log_macros.h"

#define NN_NUM_OUTPUT_BYTES         (OUT_DIM)

typedef int8_t input_tensor_t[MFCC_FIFO_BYTES];
typedef int8_t output_tensor_t[NN_NUM_OUTPUT_BYTES];

extern "C" {

/* Tensor arena buffer */
static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

/* Optional getter function for the model pointer and its size. */
extern uint8_t *GetModelPointer();

extern size_t GetModelLen();


static DSCNNModel ds_cnn_model;


void cmsis_nn_init(void) {


    if (!ds_cnn_model.Init(tensorArena,
                           sizeof(tensorArena),
                           GetModelPointer(),
                           GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return;
    }
}

int classify_on_cmsis_nn(const input_tensor_t in_data, output_tensor_t out_data) {


    TfLiteTensor *inputTensor = ds_cnn_model.GetInputTensor(0);
    uint8_t *const input_to_nn = tflite::GetTensorData<uint8_t>(inputTensor);
    memcpy(input_to_nn, in_data, MFCC_FIFO_BYTES);


    if (!ds_cnn_model.RunInference()) {
        return EE_STATUS_ERROR;
    }

    TfLiteTensor *outputTensor = ds_cnn_model.GetOutputTensor(0);
    uint8_t *const output_of_nn = tflite::GetTensorData<uint8_t>(outputTensor);


    memcpy(out_data, output_of_nn, NN_NUM_OUTPUT_BYTES);

    return EE_STATUS_OK;

}

}
