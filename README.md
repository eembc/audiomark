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

# Coding style & guidelines

TODO. EEMBC formats according to Barr-C Embedded Standards. There is a config
file for `clang-format` (v14+) [here](https://github.com/petertorelli/clang-format-barr-c).
Eventually we will move to this.

# Copyright & license

TODO. Code uploaded to this website that is already copyrighted must have a
license that allows for derivative works and does not contain copyleft. Any code
touched by EEMBC becomes copyright EEMBC and is subject to our final license.

