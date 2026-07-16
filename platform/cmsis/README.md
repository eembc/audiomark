# README

[![CMSIS AudioMark Build and FVP](https://img.shields.io/github/actions/workflow/status/FabKlein/audiomark/cmsis-audiomark-build-run.yml?logo=arm&logoColor=0091bd&label=CMSIS%20AudioMark%20Build%20and%20FVP)](../../.github/workflows/cmsis-audiomark-build-run.yml)

- How to build and run EEMBC AudioMark Applications on Arm Corstone-300/310 MPS3 FPGA, IoT kit (CM33 SSE-200), Cortex-M CMSDK or Arm Virtual Hardware.
  - The applications are intended to run on Cortex-M55/Cortex-M85 MCUs supporting Helium™ and Arm V7M-E/Arm V8.0M cores. FPU is required.
  - There are dedicated projects for running the KWS on Ethos-U NPUs.
  - Arm FPGA images and documentation can be found at https://developer.arm.com/downloads/-/download-fpga-images.
    - `AN552`: Arm® Corstone™ SSE-300 with Cortex®-M55 and Ethos™-U55 Example Subsystem for MPS3 (Partial Reconfiguration Design)
    - `AN555`: Arm® Corstone™ SSE-310 with Cortex®-M85 and Ethos™-U55 Example Subsystem for MPS3
    - `AN505`: Arm® Cortex™-M33 with IoT kit FPGA for MPS2+
    - `AN386`, `AN500`: Arm® Cortex™-M4 / Arm® Cortex™-M7 Prototyping System version 3.1 (VEM31)
  - Experimental Arm® Corstone™ SSE-315 FVP has been added. No public FPGA image is available for now.

## CMSIS Build Tools Option

See the CMSIS Toolbox installation documentation here: https://github.com/Open-CMSIS-Pack/cmsis-toolbox/blob/main/docs/installation.md


If not installed, decompress cmsis-toolbox in your workspace and set up the environment variables as described in the link above.
CMSIS Toolbox **v2.13.0** or above is required. This matches the version used
by CI in [`vcpkg-configuration.json`](vcpkg-configuration.json):
`arm:tools/open-cmsis-pack/cmsis-toolbox` = `2.13.0`.

The CMSIS Toolbox build flow uses the CMake/Ninja generator flow. The CMSIS
Toolbox manual setup instructions currently recommend CMake **3.31.5** or above
and Ninja **1.12.0** or above. For this project, the requirement can be relaxed
to CMake **3.27** or above and Ninja **1.11** or above. Some Linux
installations provide older versions, which will fail during the generated
CMSIS CMake project configuration step. This can be worked around by installing
newer CMake and Ninja versions.

All CMSIS build tools used by CI can also be installed from the local
[`vcpkg-configuration.json`](vcpkg-configuration.json), including CMSIS
Toolbox, CMake, Ninja, Arm Compiler and the Arm Virtual Hardware FVPs.

```shell
tar -zxvf cmsis-toolbox-linux64.tar.gz
PATH=$PATH:<your_cmsis_tool_path>/cmsis-toolbox-linux64/bin/

# these are optional, overriding default variables
export CMSIS_COMPILER_ROOT=<your_cmsis_tool_path>/cmsis-toolbox-linux64/etc/
export CMSIS_PACK_ROOT=<your_cmsis_pack_storage_path>cmsis-pack
```


### Toolchains and Host Tools

If not already installed, download **Arm Compiler 6.18** or later. It is
recommended to use up to date Arm Compiler 6 releases. CI currently validates
AC6 6.24, GCC 15.3.1 and LLVM Arm Toolchain for Embedded 22.1.

Make sure the host also provides CMake **3.27** or above and Ninja **1.11** or
above.

Toolchain downloads:

* Arm Compiler for Embedded: https://developer.arm.com/downloads/view/ACOMPE
* Arm GNU Toolchain: https://gitlab.arm.com/tooling/gnu-toolchains-for-arm
* LLVM Arm Toolchain for Embedded: https://github.com/arm/arm-toolchain/releases

IAR support might be added later.

For GCC Helium builds, **`avoid GCC 15.2`** which has codegen issues which are fixed with GCC 15.3.1


Following the CMSIS Toolbox installation steps, each compiler version must be
registered by defining the corresponding environment variable, for example:

```shell
export AC6_TOOLCHAIN_6_24_0=<path_to_ac6_24_compiler>/bin/
export GCC_TOOLCHAIN_15_3_1=<path_to_gcc_15_3_compiler>/bin/
export CLANG_TOOLCHAIN_22_1_0=<path_to_atfe_22_1_compiler>/bin/
```

### Initialize the new pack repository

If you did not have a local CMSIS-PACK repository, create a directory and then execute the following command in that directory:
```
cpackget init https://www.keil.com/pack/index.pidx
```
The CMSIS_PACK_ROOT variable should also point to this directory.

### Build the project

The examples below are run from the `audiomark/platform/cmsis` folder.
Passing `--packs` to `cbuild` automatically downloads missing packs, so a
separate `csolution list packs` / `cpackget add` step is not required for a
normal build.

If you only want to inspect the pack list, you can still run:

```shell
csolution list packs -s audiomark.csolution.yml -m
```

#### Build

To build the different projects, use the **cbuild** command:
e.g. for the C300 audiomark application:

```
cbuild --context audiomark_app.Release+MPS3-Corstone-300 audiomark.csolution.yml --packs --update-rte -v --toolchain AC6@6.24.0
```

This generates the application under
`audiomark/platform/cmsis/out/audiomark_app/MPS3-Corstone-300/Release/`.
AC6 builds produce an `.axf` image. GCC and CLANG builds produce an `.elf`
image.

The `--update-rte` option regenerates CMSIS RTE/configuration files. It is
useful after pack or component changes, but it may update files under `RTE/`.
For a regular rebuild after the RTE files are already current, it can be
omitted.

Similarly, GCC and CLANG generated binaries can be produced by replacing the
toolchain parameter with `GCC@x.y.z` or `CLANG@x.y.z`, where `x.y.z` is the
registered compiler version.

Expected output:
```
>> cbuild --context audiomark_app.Release+MPS3-Corstone-300 audiomark.csolution.yml --packs --update-rte -v --toolchain AC6@6.24.0
info cbuild: Build Invocation 2.13.0 (C) 2022-2026 Arm Ltd. and Contributors
info csolution: config files for each component:
  ARM::Device:Definition@1.2.0:
    - git/forks/audiomark/platform/cmsis/audiomark_app/../RTE/Device/SSE-300-MPS3/platform_base_address.h (base@1.1.2)
  ARM::Device:Startup&Baremetal@1.2.0:

...
-- Configuring done
-- Generating done
-- Build files have been written to: git/forks/audiomark/platform/cmsis/tmp/audiomark_app/MPS3-Corstone-300/Release
[1/1] Linking C executable git/forks/audiomark/platform/cmsis/out/audiomark_app/MPS3-Corstone-300/Release/audiomark_app.axf
info cbuild: build finished successfully!

```

### Running on Virtual Hardware

MPS3 Corstone-300/310 FPGA images can be run with Arm Virtual Hardware (for more information, see https://www.arm.com/products/development-tools/simulation/virtual-hardware).

With this configuration, UART output goes through a telnet terminal unless
`mps3_board.uart0.out_file=-` is used to redirect it to stdout.

To start simulation, issue the following command from the `audiomark/platform/cmsis` folder:

```
>> FVP_Corstone_SSE-300_Ethos-U55 -a out/audiomark_app/MPS3-Corstone-300/Release/audiomark_app.axf -C mps3_board.uart0.out_file=- --stat
telnetterminal1: Listening for serial connection on port 5000
telnetterminal2: Listening for serial connection on port 5001

```

**Note**
- Virtual Hardware simulation _does not provide cycle accurate measurements_ hence final audiomark score will differ from the one measured on device.


## Arm Ethos-U KWS acceleration

AudioMark KWS can be significantly accelerated by offloading the neural network processing to the Arm Ethos-U NPU engine.
The DS-CNN reference model is converted to TensorFlow Lite (TFL) format and
processed by Vela, the tool used to compile a TFL neural-network model into an
optimized version that can run on an embedded system containing an Arm Ethos-U
NPU.
Converted binary models are packed in C++ files located in the `ports/arm/`
folder. There is one file for each Ethos-U variant based on the number of MACs.
 - ds_cnn_s_quantized_U55_32_vela.tflite.cpp
 - ds_cnn_s_quantized_U55_64_vela.tflite.cpp
 - ds_cnn_s_quantized_U55_128_vela.tflite.cpp
 - ds_cnn_s_quantized_U55_256_vela.tflite.cpp
 - ds_cnn_s_quantized_U65_256_vela.tflite.cpp

Arm Corstone-300 FPGA implements Ethos-U55-128 while Corstone-310 implements Ethos-U55-256.
Arm Corstone-315 FVP implements Ethos-U65-256 or Ethos-U65-512.
Bit exactness is not affected by Ethos-U offloading.

- *Note* : `ds_cnn_s_quantized.tflite.cpp` is the MCU only variant that can be substituted to the original AudioMark model.

Building process is similar to Arm Cortex-M only builds.

```shell
# Corstone-300 FPGA/FVP build
cbuild --context audiomark_app.Release+Ethos-MPS3-Corstone-300 audiomark.csolution.yml --packs --update-rte --toolchain AC6@6.24.0

# Corstone-310 FPGA/FVP build
cbuild --context audiomark_app.Release+Ethos-MPS3-Corstone-310 audiomark.csolution.yml --packs --update-rte --toolchain AC6@6.24.0
```

Binaries can be run on FVP/Arm Virtual Hardware, but as for Cortex-M only variants, results will be different from FPGA or physical target because the simulation is not cycle-accurate.


```
>> FVP_Corstone_SSE-300_Ethos-U55 -C ethosu.num_macs=128 -a out/audiomark_app/Ethos-MPS3-Corstone-300/Release/audiomark_app.axf -C mps3_board.uart0.out_file=-
telnetterminal0: Listening for serial connection on port 5000
telnetterminal1: Listening for serial connection on port 5001
telnetterminal2: Listening for serial connection on port 5002
telnetterminal5: Listening for serial connection on port 5003

    Ethos-U rev 136b7d75 --- Apr  8 2024 21:45:17
    (C) COPYRIGHT 2019-2024 Arm Limited
    ALL RIGHTS RESERVED

Initializing
Memory alloc summary:
 bmf = 14948
 aec = 68100
 anr = 45250
 kws = 8308
INFO - Ethos-U device initialised
INFO - Ethos-U version info:
INFO - 	Arch:       v1.1.0
INFO - 	Driver:     v1.0.0
INFO - 	MACs/cc:    128
INFO - 	Cmd stream: v0
Added ethos-u support to op resolver
INFO - Creating allocator using tensor arena at 0x31000000
INFO - Allocating tensors
INFO - Model INPUT tensors:
INFO - 	tensor type is int8
INFO - 	tensor occupies 490 bytes with dimensions
INFO - 		0:   1
INFO - 		1: 490
INFO - Scale[0] = 1.084193
INFO - ZeroPoint[0] = 100
INFO - Model OUTPUT tensors:
INFO - 	tensor type is int8
INFO - 	tensor occupies 12 bytes with dimensions
INFO - 		0:   1
INFO - 		1:  12
INFO - Scale[0] = 0.003906
INFO - ZeroPoint[0] = -128
INFO - Activation buffer (a.k.a tensor arena) size used: 22548
INFO - Number of operators: 1
INFO - 	Operator 0: ethos-u
Computing run speed
[warning ][main@0][2463 ns] TA0 is not enabled!
[warning ][main@0][2463 ns] TA1 is not enabled!
Measuring
Total runtime    : 10.974 seconds
Total iterations : 10 iterations
Score            : 607.506470 AudioMarks
```


## Important Notes

 - For Corstone-300, AudioMark code and data fit entirely in I/D TCM. MPS3 FPGA system clock frequency runs at `32MHz`.
 - For Corstone-310, small TCMs prevent code and data from fitting entirely in TCM. Internal SRAM is used and benchmarks run with caches enabled. The linker scripts still apply explicit partitioning to fill TCMs with the most useful routines and data before placing the rest in SRAM; see [`linker/SSE-310-MPS3/clang_sse310_mps3.sct`](linker/SSE-310-MPS3/clang_sse310_mps3.sct) as one example. MPS3 FPGA system clock frequency runs at `25MHz`.
 - For MPS2+ Cortex-M33 IoTKit, default system clock frequency runs at `20MHz`.
 - For MPS2+ Cortex-M4/Cortex-M7, CMSDK default system clock frequency runs at `25MHz`.
 - For platforms utilizing the Ethos-U NPU and running the AudioMark DS-CNN network, it is assumed that, aside from the storage for Neural Network MFCC input and Soft Decisions output - which may be situated in cacheable memories necessitating cache maintenance operations - the rest of the shared Cortex-M and Ethos-U data are **Read-Only**. Consequently, these do not require cache clean and invalidation operations, allowing saving processing cycles. However, in the standard Ethos-U handling procedures, this approach may not be recommended. The TensorFlow Lite Micro runtime could organize and manage memory allocation in a different manner, involving write-accesses by both CPU and NPU, necessitating the use of appropriate cache operations to ensure data coherency between the CPU and NPU. For reference, typical cache and invalidation routines employed in general scenarios can be found in the ML Evaluation Kit CPU cache handling source code.
    - https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/refs/heads/main/source/hal/source/components/npu/ethosu_cpu_cache.c
