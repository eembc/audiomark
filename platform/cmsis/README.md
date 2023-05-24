# README

- How to build and run EEMBC Audiomark Applications on ARM Corstone 300 FPGA or ARM Virtual Hardware.
  - The applications will only run on Cortex-M55 MCU.
  - A dedicated project running the KWS on Ethos-U55 will be added later. Please contact ARM for more details.


## CMSIS Build tools option

See description about CMSIS Toolbox here : https://github.com/Open-CMSIS-Pack/cmsis-toolbox/blob/main/docs/installation.md


If not installed, decompress cmsis-toolbox in your workspace and setup environment variables as described in the link above

```
tar -zxvf cmsis-toolbox-linux64.tar.gz
PATH=$PATH:<your_cmsis_tool_path>/cmsis-toolbox-linux64/bin/
export CMSIS_COMPILER_ROOT=<your_cmsis_tool_path>/cmsis-toolbox-linux64/etc/
export CMSIS_PACK_ROOT=<your_cmsis_pack_storage_path>cmsis-pack
```

If not already installed, download **Arm Compiler 6.18** or later and report the tool path in : `<your_cmsis_tool_path>/cmsis-toolbox-linux64/etc/AC6.6.18.0.cmake`

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

- Remove the *GorgonMeducer::perf_counter@1.9.11* packs from this list.
- The perf_counter pack has to be downloaded from GitHub and installed separately.
- More information about the *perf_counter* library can be found here: https://github.com/GorgonMeducer/perf_counter


#### Install the packs

```
cpackget add -f required_packs.txt
```

Download *perf_counter* pack 1.9.11 from GitHub and install it:

```
cpackget add --agree-embedded-license  https://github.com/GorgonMeducer/perf_counter/raw/659eb12026977d81391d90290706f9ac3c8efe7e/cmsis-pack/GorgonMeducer.perf_counter.1.9.11.pack

```


#### Projects generation

```
csolution convert -s audiomark.csolution.yml
```

This  will generate several project files for each audiomark application:
 * *audiomark_app* : the main audiomark application
 * *testabf* : the beamformer application
 * *testaec* : the echo canceller application
 * *testanr* : the noise reductor application
 * *testkws* : the key word spotting application
 * *testmfcc* : the MFCC unit test application

For each target : FVP, C300 MPS3

Expected output:

```
audiomark/platform/cmsis/audiomark_app.Release+FVP.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/audiomark_app.Release+MPS3-Corstone-300.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf.Release+FVP.cprj - info csolution: file generated successfully
audiomark/platform/cmsis/testabf.Release+MPS3-Corstone-300.cprj - info csolution: file generated successfully
...
<full list eluded>
...
audiomark/platform/cmsis/testkws.Release+FVP.cbuild.yml - info csolution: file generated successfully
audiomark/platform/cmsis/testkws.Release+MPS3-Corstone-300.cbuild.yml - info csolution: file generated successfully
```


#### Building the cprj files

To build the different projects, use the **cbuild** command:
e.g. for the C300 audiomark application:
 
```
cbuild audiomark_app.Release+MPS3-Corstone-300.cprj
```

This will generate an bject in `audiomark/platform/cmsis/out/audiomark_app/MPS3-Corstone-300/Release/Release+MPS3-Corstone-300.axf`


Expected output:
```
>> cbuild audiomark_app.Release+MPS3-Corstone-300.cprj
info cbuild: Build Invocation 1.5.0 (C) 2023 Arm Ltd. and Contributors
M650: Command completed successfully.

M652: Generated file for project build: 'git/audiomark-dev/platform/cmsis/tmp/audiomark_app/MPS3-Corstone-300/Release/CMakeLists.txt'
-- Configuring done
-- Generating done
-- Build files have been written to: git/audiomark-dev/platform/cmsis/tmp/audiomark_app/MPS3-Corstone-300/Release
[126/126] Linking C executable git/audiomark-dev/platform/cmsis/out/audiomark_app/MPS3-Corstone-300/Release/audiomark_app.Release+MPS3-Corstone-300.axf
"git/audiomark-dev/platform/cmsis/RTE/Device/SSE-300-MPS3/fvp_sse300_mps3_s.sct", line 24 (column 11): Warning: L6314W: No section matches pattern *(Veneer$$CMSE).
Finished: 0 information, 1 warning and 0 error messages.
info cbuild: build finished successfully!

```

#### Running on Virtual Hardware

MP3 Corstone 300 FPGA image can be run with the ARM Virtual Hardware (For more information, please refer to https://www.arm.com/products/development-tools/simulation/virtual-hardware)

With this configuration, UART output are going through a telnet terminal.

This can be changed by removing printf_mps3.clayer.yml or create a dedicated target in the audiomark.csolution.yml.

To start simulation, issue following command from audiomark/platform/cmsis folder

```
>> VHT_MPS3_Corstone_SSE-300 ./out/audiomark_app/MPS3-Corstone-300/Release/Release+MPS3-Corstone-300.axf -f model_config_sse300.txt  --stat
telnetterminal1: Listening for serial connection on port 5000
telnetterminal2: Listening for serial connection on port 5001

```

**Important Note :**

Virtual Hardware simulation _does not provide cycle accurate measurements_ hence final audiomark score will differ from the one measured on device.


## Keil MDK Build

As for the CMSIS Build variant, perf_counter library has to be installed manually for now
It can be retrieved from :
https://github.com/GorgonMeducer/perf_counter/raw/659eb12026977d81391d90290706f9ac3c8efe7e/cmsis-pack/GorgonMeducer.perf_counter.1.9.11.pack

More information about the *perf_counter* library can be found here: https://github.com/GorgonMeducer/perf_counter

The Audiomark main application can be build by loading the **audiomark_app.Release+MPS3-Corstone-300.uvoptx** uVision project.
It can be noted that this one has been generated by importing the **audiomark_app.Release+MPS3-Corstone-300.cprj** in uVision (Project=>Import)

The printf messages are output to MPS3 FPGA board 2nd UART port (settings 115200, 8,N,1) by this project default setting. To change printf messages to Debug (printf) Viewer window
using the tracing method through JTAG, please select ITM for STDOUT under Compiler => I/O in Manage Run-Time Environment menu.

MDK Projects for the various audiomark components can easily be created by importing the different cprojects files generated by CMSIS Build csolution (see Convert csolution above)

