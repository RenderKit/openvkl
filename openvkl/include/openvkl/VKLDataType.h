// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if __cplusplus >= 201103L
#include <cstdint>
#endif

// this header is shared with ISPC

// An enum type that represensts the different data types represented in openvkl
//
// IMPORTANT: enums added here should also be represented in stringForType() and
// other functions in VKLCommon.cpp!
#if __cplusplus >= 201103L
typedef enum : uint32_t
#else
typedef enum
#endif
{
  // Driver reference type.
  VKL_DRIVER = 100,

  // Void pointer type.
  VKL_VOID_PTR = 200,

  // Booleans, same size as VKL_INT.
  VKL_BOOL = 250,

  // highest bit to represent objects/handles
  VKL_OBJECT = 0x8000000,

  // Object reference subtypes.
  VKL_DATA = 0x8000000 + 100,
  VKL_VALUE_SELECTOR,
  VKL_VOLUME,

  // Pointer to a C-style NULL-terminated character string.
  VKL_STRING = 1500,

  // Character scalar type.
  VKL_CHAR = 2000,

  // Unsigned character scalar and vector types.
  VKL_UCHAR = 2500, VKL_VEC2UC, VKL_VEC3UC, VKL_VEC4UC,
  VKL_BYTE = 2500, //XXX VKL_UCHAR, ISPC issue #1246
  VKL_RAW = 2500,  //XXX VKL_UCHAR, ISPC issue #1246

  // Signed 16-bit integer scalar.
  VKL_SHORT = 3000,

  // Unsigned 16-bit integer scalar.
  VKL_USHORT = 3500,

  // Signed 32-bit integer scalar and vector types.
  VKL_INT = 4000, VKL_VEC2I, VKL_VEC3I, VKL_VEC4I,

  // Unsigned 32-bit integer scalar and vector types.
  VKL_UINT = 4500, VKL_VEC2UI, VKL_VEC3UI, VKL_VEC4UI,

  // Signed 64-bit integer scalar and vector types.
  VKL_LONG = 5000, VKL_VEC2L, VKL_VEC3L, VKL_VEC4L,

  // Unsigned 64-bit integer scalar and vector types.
  VKL_ULONG = 5550, VKL_VEC2UL, VKL_VEC3UL, VKL_VEC4UL,

  // Single precision floating point scalar and vector types.
  VKL_FLOAT = 6000, VKL_VEC2F, VKL_VEC3F, VKL_VEC4F,

  // Double precision floating point scalar type.
  VKL_DOUBLE = 7000,

  // Signed 32-bit integer N-dimensional box types
  VKL_BOX1I = 8000, VKL_BOX2I, VKL_BOX3I, VKL_BOX4I,

  // Single precision floating point N-dimensional box types
  VKL_BOX1F = 10000, VKL_BOX2F, VKL_BOX3F, VKL_BOX4F,

  // Transformation types
  VKL_LINEAR2F = 12000, VKL_LINEAR3F, VKL_AFFINE2F, VKL_AFFINE3F,

  // Guard value.
  VKL_UNKNOWN = 9999999
} VKLDataType;
