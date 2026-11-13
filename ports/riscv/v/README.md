# AudioMark - RISC-V Vector Port

## Overview

This directory contains an implementation of selected DSP and NN kernels targeting
the RISC-V Vector Extension (RVV) version 1.0 using the standard RVV C intrinsics.

The implementation follows the ratified RVV 1.0 specification and is designed to
operate across the standard vector extension as well as embedded Zve profiles.

Reference intrinsics specification:

https://docs.riscv.org/reference/vector-c-intrinsics/_attachments/v-intrinsic-spec.pdf

---

## Toolchain & Dependencies

This implementation requires compiler support for the RVV 1.0 intrinsics API.

### GCC

- GCC 14 or newer.

  GCC 13 and earlier do not provide the vector tuple type support required by the
  RVV 1.0 intrinsics interface.

### LLVM/Clang

- Clang 19 or newer.

---

## Supported Architectures

This implementation has been tested with the full `V` extension, `Zve32x`, `Zve32f`, `Zve64x` and `Zve64f`.

The implementation is vector-length agnostic (VLA) and relies only on the standard
RVV 1.0 programming model.

---

## Vector Floating-Point Support

Some kernels make use of RVV single-precision floating-point instructions.

During CMake configuration, compiler support for vector floating-point intrinsics is
detected automatically.

If vector floating-point support is unavailable (for example when targeting
`Zve32x`), those kernels automatically fall back to their scalar floating-point
implementations while all integer vectorized kernels continue to use RVV.

When vector floating-point support is available (for example on `Zve32f` or
targets with the full `V` extension), the RVV floating-point implementations are
selected.

No additional CMake options or compiler flags are required to enable this
selection.

## Known Issues

A limitation in GCC 15.2 and earlier causes the MDF tests to fail for some configurations of optimization flags.
The issue has been fixed in GCC 16 and has since been backported to GCC 15.3 (see [GCC PR 122448](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122448)).
