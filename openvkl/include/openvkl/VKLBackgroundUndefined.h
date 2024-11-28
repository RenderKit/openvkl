// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// A special value we use to distinguish an undefined field value. This could
// be the result of sampling out of bounds, or sampling at a position in bounds
// but not covered by any input data.
// This value is a quiet NaN.

#ifdef ISPC
#define vkl_floatbits floatbits
#else
#if defined(__cplusplus)
#include <cstdint>
#include <cstring>
using std::memcpy;
#else
#include <stdint.h>
#include <string.h>
#endif
inline float vkl_floatbits(uint32_t bits)
{
  float fval;
  memcpy(&fval, &bits, 4);
  return fval;
}
#endif

#define VKL_BACKGROUND_UNDEFINED vkl_floatbits(0xFFC068B5u)
