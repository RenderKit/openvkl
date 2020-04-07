// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ========================================================================== //
// In this header, we define a number of macros and typedefs to facilitate
// shared C++ / ISPC source files.
// ========================================================================== //

#pragma once

// ========================================================================== //
#if defined(ISPC)
// ========================================================================== //

#include "common.isph"

// Keywords.
#define VKL_INTEROP_UNIFORM uniform
#define VKL_INTEROP_CONSTEXPR

// Unified integer typedefs.

typedef unsigned int vkl_uint32;
typedef int vkl_int32;
typedef unsigned int64 vkl_uint64;

// Call macros for both varying and uniform types.
#define __vkl_interop_univary(Macro) Macro(uniform) Macro(varying)

// ========================================================================== //
#elif defined(__cplusplus)
// ========================================================================== //

#include <cassert>
#include <cstdint>

// Keywords.
#define VKL_INTEROP_UNIFORM
#define VKL_INTEROP_CONSTEXPR constexpr

// Unified integer typedefs.

typedef uint32_t vkl_uint32;
typedef int32_t vkl_int32;
typedef uint64_t vkl_uint64;

// ISPC library functions missing from C++

inline float floatbits(vkl_uint32 bits)
{
  static_assert(sizeof(float) == sizeof(vkl_uint32), "Float is not 4 Bytes.");
  float fval                         = 0.f;
  reinterpret_cast<char *>(&fval)[0] = reinterpret_cast<const char *>(&bits)[0];
  reinterpret_cast<char *>(&fval)[1] = reinterpret_cast<const char *>(&bits)[1];
  reinterpret_cast<char *>(&fval)[2] = reinterpret_cast<const char *>(&bits)[2];
  reinterpret_cast<char *>(&fval)[3] = reinterpret_cast<const char *>(&bits)[3];
  return fval;
}

inline vkl_uint32 intbits(float value)
{
  static_assert(sizeof(float) == sizeof(vkl_uint32), "Float is not 4 Bytes.");
  vkl_uint32 ival = 0;
  reinterpret_cast<char *>(&ival)[0] =
      reinterpret_cast<const char *>(&value)[0];
  reinterpret_cast<char *>(&ival)[1] =
      reinterpret_cast<const char *>(&value)[1];
  reinterpret_cast<char *>(&ival)[2] =
      reinterpret_cast<const char *>(&value)[2];
  reinterpret_cast<char *>(&ival)[3] =
      reinterpret_cast<const char *>(&value)[3];
  return ival;
}

// Call macros for both varying and uniform types.
#define __vkl_interop_univary(Macro) Macro(VKL_INTEROP_UNIFORM)

// ========================================================================== //
#endif  // defined(ISPC)
// ========================================================================== //
