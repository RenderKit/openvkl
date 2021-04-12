// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#define VKL_DEFAULT_VAL(a) a
#else
#define VKL_DEFAULT_VAL(a)
#endif

#ifdef _WIN32
#  define VKL_ALIGN(...) __declspec(align(__VA_ARGS__))
#else
#  define VKL_ALIGN(...) __attribute__((aligned(__VA_ARGS__)))
#endif

#ifdef _WIN32
#  ifdef openvkl_EXPORTS
#    define OPENVKL_INTERFACE __declspec(dllexport)
#  else
#    define OPENVKL_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OPENVKL_INTERFACE
#endif

#ifdef __GNUC__
#  define OPENVKL_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#  define OPENVKL_DEPRECATED __declspec(deprecated)
#else
#  define OPENVKL_DEPRECATED
#endif

#ifdef __cplusplus
struct ManagedObject
{
};
#else
typedef void ManagedObject;
#endif

typedef ManagedObject *VKLObject;

typedef struct
{
  int x, y, z;
} vkl_vec3i;

typedef struct
{
  float x, y, z;
} vkl_vec3f;

typedef struct
{
  float lower, upper;
} vkl_range1f;

typedef struct
{
  vkl_vec3i lower, upper;
} vkl_box3i;

typedef struct
{
  vkl_vec3f lower, upper;
} vkl_box3f;

#define __define_wide_types(WIDTH, ALIGNMENT) \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    float x[WIDTH];                           \
    float y[WIDTH];                           \
    float z[WIDTH];                           \
  } vkl_vvec3f##WIDTH;                        \
                                              \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    float lower[WIDTH], upper[WIDTH];         \
  } vkl_vrange1f##WIDTH;

__define_wide_types(4, 16);
__define_wide_types(8, 32);
__define_wide_types(16, 64);

#undef __define_wide_types

/*
 * Force expansion on two tokens and concatenate them.
 */

#define __vkl_concat(A, B) __vkl_concat_impl(A, B)
#define __vkl_concat_impl(A, B) A##B
