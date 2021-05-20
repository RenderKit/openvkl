// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h"

#ifdef _WIN32
#define VKL_ALIGN(...) __declspec(align(__VA_ARGS__))
#else
#define VKL_ALIGN(...) __attribute__((aligned(__VA_ARGS__)))
#endif

#ifdef _WIN32
#ifdef openvkl_EXPORTS
#define OPENVKL_INTERFACE __declspec(dllexport)
#else
#define OPENVKL_INTERFACE __declspec(dllimport)
#endif
#else
#define OPENVKL_INTERFACE
#endif

#define __define_wide_types(WIDTH, ALIGNMENT) \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    vkl_float x[WIDTH];                       \
    vkl_float y[WIDTH];                       \
    vkl_float z[WIDTH];                       \
  } vkl_vvec3f##WIDTH;                        \
                                              \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    vkl_float lower[WIDTH], upper[WIDTH];     \
  } vkl_vrange1f##WIDTH;

__define_wide_types(4, 16);
__define_wide_types(8, 32);
__define_wide_types(16, 64);

#undef __define_wide_types

// Force expansion on two tokens and concatenate them.
#define __vkl_concat(A, B) __vkl_concat_impl(A, B)
#define __vkl_concat_impl(A, B) A##B
