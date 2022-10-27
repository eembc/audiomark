# Introduction

This is the development repository for AudioMark. It is based on an architecture
from Arm (Laurent Le Faucheur) based on beamforming and echo cancellation. The
intent is to review and expand this code base with the remaining blocks in the
architecture diagram (see minutes) until we have a complete benchmark.

# Reviewing

A few meetings ago we made a quick list of things to keep in mind during review:

* does it require too much memory, 
* or asynchronous threading, 
* or it has too many types of instruction sets that h/w doesn’t support,
* or are trivial functions used where API functions should be used,
* and pay attention to iterative calculations to make sure they are not redundant (multiplying with a constant in a for-loop

Feel free to file an issue.

# POC vs. Beta

The proof of concept is the shortest path to the minumum viable product. At
EEMBC, this usually happens in the first 6-9 months of development. It is then
followed by "beta" where we must test on 3-5 different architectures and
compilers to guarantee that it will work on all EEMBC members' hardware in an
optimal way. Right now we are in POC, but please don't hesitate to bring this
up on your system today and report questions or issues.

# Unit-testing for pull requests

TODO. Eventually we will need to start adding `make`-friendly unit-tests to
automate code reviews and CI-on-PR via GitHUb.

# Building

## cmake

There is a `platforms/cmake` area which contains a `CMakeList.txt` file that works with
GCC, macOS Clang, Cygwin, and MSVC. The first three platforms' `cmake` generate a standard
`Makefile` for use with `make`. Compiling with MSVC `cmake` produces a solution
file `audiomark.sln` which can be opened from the MSVC IDE and compiled/debugged.

# Coding style & guidelines

## Formatting

EEMBC formats according to Barr-C Embedded Standards. The `.clang-format` file
in the root directory observes this. This file can be used within VSCode or
MSVC, however it isn't clear if this behaves the same as `clang-format`
version 14 (which aligns pointer stars differently).

## Function and file naming

Traditionally, all functions and files start with either `ee_` or `th_`, where
the former notation indicates "thou shall not change" and the latter must be
changed in order to port the code (i.e., the Test Harness, hence `th_`).

Since there is so much code from Xiph that we are including, we will not change
all of their code but might have to change some (like FFT wrappers). It may
require a Run Rule to avoid this code being altered. Ideally every function
should fall into a simple `th_api` folder or collection of files so that it is
obvious what needs to be ported. Currently the `components/eembc` folder
illustrates this by separating all of the Arm-specific code into `th_api.c`.

# Copyright & license

TODO. Code uploaded to this website that is already copyrighted must have a
license that allows for derivative works and does not contain copyleft. Any code
touched by EEMBC becomes copyright EEMBC and is subject to our final license.

