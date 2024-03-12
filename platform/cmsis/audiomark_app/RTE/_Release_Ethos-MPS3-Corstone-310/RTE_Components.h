
/*
 * Auto generated Run-Time-Environment Configuration File
 *      *** Do not modify ! ***
 *
 * Project: 'audiomark_app.Release+Ethos-MPS3-Corstone-310' 
 * Target:  'Release+Ethos-MPS3-Corstone-310' 
 */

#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H


/*
 * Define the Device Header File: 
 */
#define CMSIS_device_header "SSE310MPS3.h"

/* ARM::CMSIS Driver:USART@1.0.0 */
#define RTE_Drivers_USART
/* Arm::Machine Learning:NPU Support:Ethos-U Driver&Generic U55@1.22.8 */
#define RTE_ETHOS_U_CORE_DRIVER
/* GorgonMeducer::Utilities&Performance Counter:perf_counter:Core&Library@1.9.11 */
#define __PERF_COUNTER__
/* Keil::Compiler&ARM Compiler:I/O:STDOUT&User@1.2.0 */
#define RTE_Compiler_IO_STDOUT          /* Compiler I/O: STDOUT */
          #define RTE_Compiler_IO_STDOUT_User     /* Compiler I/O: STDOUT User */
/* tensorflow::Data Exchange:Serialization:flatbuffers&tensorflow@1.22.8 */
#define RTE_DataExchange_Serialization_flatbuffers     /* flatbuffers */
/* tensorflow::Data Processing:Math:gemmlowp fixed-point&tensorflow@1.22.8 */
#define RTE_DataExchange_Math_gemmlowp     /* gemmlowp */
/* tensorflow::Data Processing:Math:kissfft&tensorflow@1.22.8 */
#define RTE_DataExchange_Math_kissfft     /* kissfft */
/* tensorflow::Data Processing:Math:ruy&tensorflow@1.22.8 */
#define RTE_DataProcessing_Math_ruy     /* ruy */
/* tensorflow::Machine Learning:TensorFlow:Kernel&Ethos-U@1.22.8 */
#define RTE_ML_TF_LITE     /* TF */


#endif /* RTE_COMPONENTS_H */
