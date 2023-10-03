# README

- How to build and run EEMBC Audiomark Applications on ARM Corstone-300/310 MPS3 FPGA, IoT kit, Cortex-M CMSDK or ARM Virtual Hardware.
  - The applications are intended to run on Cortex-M55/Cortex-M85 MCUs supporting Helium™ and Arm V7M-E/Arm V8.0M cores. FPU is required.
  - Dedicated projects running the KWS on Ethos-U55 NPU will be added later. Please contact ARM for more details.
  - ARM FPGA images and documentation can be found at https://developer.arm.com/downloads/-/download-fpga-images.
    - `AN552`: Arm® Corstone™ SSE-300 with Cortex®-M55 and Ethos™-U55 Example Subsystem for MPS3 (Partial Reconfiguration Design)
    - `AN555`: Arm® Corstone™ SSE-310 with Cortex®-M85 and Ethos™-U55 Example Subsystem for MPS3
    - `AN505`: Arm® Cortex™-M33 with IoT kit FPGA for MPS2+
    - `AN386`, `AN500`: Arm® Cortex™-M4 / Arm® Cortex™-M7 Prototyping System version 3.1 (VEM31)

## CMSIS Build tools option

If only considering building the benchmarks with Keil MDK without CMSIS Toolbox, following paragraph can be skipped and move directly [here](#keil-mdk-builds).

See description about CMSIS Toolbox here: https://github.com/Open-CMSIS-Pack/cmsis-toolbox/blob/main/docs/installation.md


If not installed, decompress cmsis-toolbox in your workspace and setup environment variables as described in the link above.
CMSIS Toolbox v2.0.0 or above is required.

```
tar -zxvf cmsis-toolbox-linux64.tar.gz
PATH=$PATH:<your_cmsis_tool_path>/cmsis-toolbox-linux64/bin/
export CMSIS_COMPILER_ROOT=<your_cmsis_tool_path>/cmsis-toolbox-linux64/etc/
export CMSIS_PACK_ROOT=<your_cmsis_pack_storage_path>cmsis-pack
```

If not already installed, download **Arm Compiler 6.18** or later and report the tool path in : `<your_cmsis_tool_path>/cmsis-toolbox-linux64/etc/AC6.6.18.0.cmake`
Support for other toolchains like CLANG, GCC or IAR will be added later.


```makefile
 ############### EDIT BELOW ###############
# Set base directory of toolchain
set(TOOLCHAIN_ROOT "<arm_compiler_root_path>/bin/")
set(EXT)

############ DO NOT EDIT BELOW ###########
```


### Initialize the new pack repository

```
cpackget init https://www.keil.com/pack/index.pidx
```


### Build the project

#### Generate list of needed packs

From this `audiomark/platform/cmsis` folder, type the command:

```
csolution list packs -s audiomark.csolution.yml -m > required_packs.txt
```



#### Install the packs

```
cpackget add -f required_packs.txt
```


#### Projects generation

```
csolution convert -s audiomark.csolution.yml -t AC6
```

This  will generate several project files for each audiomark application:
 * *audiomark_app* : the main audiomark application
 * *testabf* : the beamformer application
 * *testaec* : the echo canceller application
 * *testanr* : the noise reductor application
 * *testkws* : the key word spotting application
 * *testmfcc* : the MFCC unit test application

For each target : FVP, C300 & C310 MPS3, CM4, CM7 and CM33 MPS2

Expected output:

```
audiomark/platform/cmsis/testanr/testanr.Release+MPS3-Corstone-300.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testanr/testanr.Release+MPS3-Corstone-310.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testanr/testanr.Release+VHT-Corstone-300.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testanr/testanr.Release+VHT-Corstone-310.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf/testabf.Release+MPS3-Corstone-300.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf/testabf.Release+MPS3-Corstone-310.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf/testabf.Release+VHT-Corstone-300.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf/testabf.Release+VHT-Corstone-310.cprj - info csolution: file generated successfully
...
<full list eluded>
...
```


#### Building the cprj files

To build the different projects, use the **cbuild** command:
e.g. for the C300 audiomark application:

```
cbuild audiomark_app.Release+MPS3-Corstone-300.cprj
```
or
```
cbuild --context audiomark_app.Release+MPS3-Corstone-300 audiomark.csolution.yml  --update-rte -v --toolchain AC6@6.20
```

This will generate an object in `audiomark/platform/cmsis/out/audiomark_app/MPS3-Corstone-300/Release/audiomark_app.axf`


Expected output:
```
>> cbuild --context audiomark_app.Release+MPS3-Corstone-300 audiomark.csolution.yml  --update-rte -v --toolchain AC6@6.20
info cbuild: Build Invocation 2.1.0 (C) 2023 Arm Ltd. and Contributors
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

MP3 Corstone-300/310 FPGA image can be run with the ARM Virtual Hardware (For more information, please refer to https://www.arm.com/products/development-tools/simulation/virtual-hardware)

With this configuration, UART output are going through a telnet terminal.

There are `VHT`-prefixed projects variants like **audiomark_app.Release+VHT-Corstone-300.cprj**, similar to MPS3 ones, with the exception of the console handling using semi-hosting instead of UART. This can be useful when running on alternative simulation environments.


To start simulation, issue following command from audiomark/platform/cmsis folder

```
>> VHT_MPS3_Corstone_SSE-300 out/audiomark_app/MPS3-Corstone-300/Release/audiomark_app.axf -f model_config_sse300.txt  --stat
telnetterminal1: Listening for serial connection on port 5000
telnetterminal2: Listening for serial connection on port 5001

```

**Note**
- Virtual Hardware simulation _does not provide cycle accurate measurements_ hence final audiomark score will differ from the one measured on device.


## Keil MDK Builds

The Audiomark Corstone-300 FPGA main application can be built by importing the **audiomark_app/audiomark_app.Release+MPS3-Corstone-300.cprj** in uVision (Project=>Import)

- The printf messages are output to MPS3 FPGA board 2nd UART port (settings `115200, 8,N,1`) by project default setting.
- To change printf messages to Debug (printf) Viewer window using the tracing method through JTAG, please select ITM for STDOUT under Compiler => I/O in Manage Run-Time Environment menu.

Various individual audiomark components unit-tests project can imported using the different cprojects files provided in this folder (e.g. *testanr.Release+MPS3-Corstone-300.cprj* for Noise suppressor Corstone-300 Unit test)

For Corstone-310 FPGA, similar steps can be followed by importing **audiomark_app/audiomark_app.Release+MPS3-Corstone-310.cprj** and / or different unit tests

For Arm V7M-E/ Arm V8.0M MPS2+ FPGA, similar steps can be followed by importing **audiomark_app/audiomark_app.Release+MPS2-IOTKit-CM33.cprj**, **audiomark_app/audiomark_app.Release+MPS2-CMSDK_CM7_SP.cprj**, **audiomark_app/audiomark_app.Release+MPS2-CMSDK_CM4_FP.cprj** and / or different unit tests

For Virtual Hardware audiomark components, import projects having `VHT` prefix like **testaec/testaec.Release+VHT-Corstone-300.cprj**.


## Important Notes

 - For Corstone-300, Audiomark Code and Data fit entierely in I/D TCM. MPS3 FPGA system clock frequency runs at `32Mhz`
 - For Corstone-310, small TCMs prevent Code and Data to fit in these. Internal SRAM are used and benchmarks will run with caches enabled. MPS3 FPGA system clock frequency runs at `25Mhz`
 - For MPS2+ Cortex-M33 IoTKit, default system clock frequency runs at `20Mhz`
 - For MPS2+ Cortex-M4/Cortex-M7, CMSDK default system clock frequency runs at `25Mhz`
