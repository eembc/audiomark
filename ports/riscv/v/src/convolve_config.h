/**
 * Copyright 2026 Sohail Raj Satapathy
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NN_CONVOLVE_CONFIG_H
#define NN_CONVOLVE_CONFIG_H

/* Number of im2col columns buffered before dispatching to the matrix-multiply
 * kernel. Set to match the kernel's row count.
 */
#define NN_KERNEL_COLS 7

#define DS_CNN_S_LAYER_1_CONV2D_WEIGHTS ds_cnn_s_layer_1_conv2d_weights_rvv
#define DS_CNN_S_LAYER_3_CONV2D_WEIGHTS ds_cnn_s_layer_3_conv2d_weights_rvv
#define DS_CNN_S_LAYER_5_CONV2D_WEIGHTS ds_cnn_s_layer_5_conv2d_weights_rvv
#define DS_CNN_S_LAYER_7_CONV2D_WEIGHTS ds_cnn_s_layer_7_conv2d_weights_rvv
#define DS_CNN_S_LAYER_9_CONV2D_WEIGHTS ds_cnn_s_layer_9_conv2d_weights_rvv

#define NN_CONV0_S8(n)                               \
    nn_conv0_s8(&ctx,                                \
                &quant_params,                       \
                in_data,                             \
                DS_CNN_S_LAYER_##n##_CONV2D_WEIGHTS, \
                ds_cnn_s_layer_##n##_conv2d_bias,    \
                in_out_buf_0)

#define NN_CONV1X1_S8(n)                               \
    nn_conv1x1_s8(&ctx,                                \
                  &conv_params,                        \
                  &quant_params,                       \
                  in_out_buf_1,                        \
                  DS_CNN_S_LAYER_##n##_CONV2D_WEIGHTS, \
                  ds_cnn_s_layer_##n##_conv2d_bias,    \
                  in_out_buf_0)

#endif
