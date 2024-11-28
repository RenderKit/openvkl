// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// -----------------------------------------------------------------------------
// In this header, we define a number of macros and typedefs to facilitate
// shared C / C++ / ISPC source files.
// -----------------------------------------------------------------------------

#pragma once

#include "types.h"

// -----------------------------------------------------------------------------
// Keywords.
// -----------------------------------------------------------------------------

#if defined(ISPC)

#define VKL_INTEROP_CONSTEXPR
#define VKL_INTEROP_UNIFORM uniform
#define VKL_INTEROP_VARYING varying
#else  // defined(ISPC)

#if defined(__cplusplus)
#define VKL_INTEROP_CONSTEXPR constexpr
#else
#define VKL_INTEROP_CONSTEXPR
#endif

#define VKL_INTEROP_UNIFORM
#define VKL_INTEROP_VARYING

#endif  // defined(ISPC)

// -----------------------------------------------------------------------------
// Deprecation warnings.
// -----------------------------------------------------------------------------

#ifdef __GNUC__
#define VKL_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define VKL_DEPRECATED __declspec(deprecated)
#else
#define VKL_DEPRECATED
#endif

// -----------------------------------------------------------------------------
// Assertions.
// -----------------------------------------------------------------------------

#if defined(ISPC)

#define VKL_INTEROP_STATIC_ASSERT(condition, message)

#else

#if defined(__cplusplus)
#include <cassert>
#else
#include <assert.h>
#endif

#define VKL_INTEROP_STATIC_ASSERT(condition, message) \
  static_assert((condition), message)

#endif

// -----------------------------------------------------------------------------
// Default parameters / initialization.
// -----------------------------------------------------------------------------

#if defined(__cplusplus)

#define VKL_DEFAULT_VAL(a) a

#else  // defined(__cplusplus)

#define VKL_DEFAULT_VAL(a)

#endif  // defined(__cplusplus)

// -----------------------------------------------------------------------------
// Helpers for univary definitions.
// -----------------------------------------------------------------------------

#if defined(ISPC)

// Call Macro for both varying and uniform types.
#define __vkl_interop_univary(Macro) Macro(uniform) Macro(varying)

#else

// Call Macro _only_ for uniform types.
#define __vkl_interop_univary(Macro) Macro(VKL_INTEROP_UNIFORM)

#endif
