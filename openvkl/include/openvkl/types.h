// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// -----------------------------------------------------------------------------
// This is a shared ISPC, C and C++ header, which contains typedefs and data
// structures for most enums in VKLDataType.
// -----------------------------------------------------------------------------

#pragma once

// -----------------------------------------------------------------------------
// Basic types
// -----------------------------------------------------------------------------

#if defined(ISPC)

typedef bool vkl_bool;

typedef int8 vkl_int8;
typedef int16 vkl_int16;
typedef int vkl_int32;
typedef int64 vkl_int64;

typedef unsigned int8 vkl_uint8;
typedef unsigned int16 vkl_uint16;
typedef unsigned int vkl_uint32;
typedef unsigned int64 vkl_uint64;

#else  // defined(ISPC)

#if defined(__cplusplus)
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

typedef bool vkl_bool;

typedef int8_t vkl_int8;
typedef int16_t vkl_int16;
typedef int32_t vkl_int32;
typedef int64_t vkl_int64;

typedef uint8_t vkl_uint8;
typedef uint16_t vkl_uint16;
typedef uint32_t vkl_uint32;
typedef uint64_t vkl_uint64;

#endif  // defined(ISPC)

// -----------------------------------------------------------------------------
// VKL_OBJECT
// -----------------------------------------------------------------------------

// TODO: rename this struct to VKLObject once full conversion is done
typedef struct __APIObject
{
#ifdef __cplusplus
  __APIObject() : host(nullptr), device(nullptr) {}

  operator bool() const
  {
    return (host != nullptr);
  }
#endif

  void *host;
  void *device;
} APIObject;

typedef APIObject VKLObject;

// -----------------------------------------------------------------------------
// VKL_CHAR
// -----------------------------------------------------------------------------

typedef vkl_int8 vkl_char;

typedef struct __vkl_vec2c
{
  vkl_char x, y;
} vkl_vec2c;

typedef struct __vkl_vec3c
{
  vkl_char x, y, z;
} vkl_vec3c;

typedef struct __vkl_vec4c
{
  vkl_char x, y, z, w;
} vkl_vec4c;

// -----------------------------------------------------------------------------
// VKL_UCHAR
// -----------------------------------------------------------------------------

typedef vkl_uint8 vkl_uchar;

typedef struct __vkl_vec2uc
{
  vkl_uchar x, y;
} vkl_vec2uc;

typedef struct __vkl_vec3uc
{
  vkl_uchar x, y, z;
} vkl_vec3uc;

typedef struct __vkl_vec4uc
{
  vkl_uchar x, y, z, w;
} vkl_vec4uc;

typedef vkl_uint8 vkl_byte;
typedef vkl_uint8 vkl_raw;

// -----------------------------------------------------------------------------
// VKL_SHORT
// -----------------------------------------------------------------------------

typedef vkl_int16 vkl_short;

typedef struct __vkl_vec2s
{
  vkl_short x, y;
} vkl_vec2s;

typedef struct __vkl_vec3s
{
  vkl_short x, y, z;
} vkl_vec3s;

typedef struct __vkl_vec4s
{
  vkl_short x, y, z, w;
} vkl_vec4s;

// -----------------------------------------------------------------------------
// VKL_USHORT
// -----------------------------------------------------------------------------

typedef vkl_uint16 vkl_ushort;

typedef struct __vkl_vec2us
{
  vkl_ushort x, y;
} vkl_vec2us;

typedef struct __vkl_vec3us
{
  vkl_ushort x, y, z;
} vkl_vec3us;

typedef struct __vkl_vec4us
{
  vkl_ushort x, y, z, w;
} vkl_vec4us;

// -----------------------------------------------------------------------------
// VKL_INT
// -----------------------------------------------------------------------------

typedef vkl_int32 vkl_int;

typedef struct __vkl_vec2i
{
  vkl_int x, y;
} vkl_vec2i;

typedef struct __vkl_vec3i
{
  vkl_int x, y, z;
} vkl_vec3i;

typedef struct __vkl_vec4i
{
  vkl_int x, y, z, w;
} vkl_vec4i;

// -----------------------------------------------------------------------------
// VKL_UINT
// -----------------------------------------------------------------------------

typedef vkl_uint32 vkl_uint;

typedef struct __vkl_vec2ui
{
  vkl_uint x, y;
} vkl_vec2ui;

typedef struct __vkl_vec3ui
{
  vkl_uint x, y, z;
} vkl_vec3ui;

typedef struct __vkl_vec4ui
{
  vkl_uint x, y, z, w;
} vkl_vec4ui;

// -----------------------------------------------------------------------------
// VKL_LONG
// -----------------------------------------------------------------------------

typedef vkl_int64 vkl_long;

typedef struct __vkl_vec2l
{
  vkl_long x, y;
} vkl_vec2l;

typedef struct __vkl_vec3l
{
  vkl_long x, y, z;
} vkl_vec3l;

typedef struct __vkl_vec4l
{
  vkl_long x, y, z, w;
} vkl_vec4l;

// -----------------------------------------------------------------------------
// VKL_ULONG
// -----------------------------------------------------------------------------

typedef vkl_uint64 vkl_ulong;

typedef struct __vkl_vec2ul
{
  vkl_ulong x, y;
} vkl_vec2ul;

typedef struct __vkl_vec3ul
{
  vkl_ulong x, y, z;
} vkl_vec3ul;

typedef struct __vkl_vec4ul
{
  vkl_ulong x, y, z, w;
} vkl_vec4ul;

// -----------------------------------------------------------------------------
// VKL_HALF
// -----------------------------------------------------------------------------

typedef vkl_uint16 vkl_half;

typedef struct __vkl_vec2h
{
  vkl_half x, y;
} vkl_vec2h;

typedef struct __vkl_vec3h
{
  vkl_half x, y, z;
} vkl_vec3h;

typedef struct __vkl_vec4h
{
  vkl_half x, y, z, w;
} vkl_vec4h;

// -----------------------------------------------------------------------------
// VKL_FLOAT
// -----------------------------------------------------------------------------

typedef float vkl_float;

typedef struct __vkl_vec2f
{
  vkl_float x, y;
} vkl_vec2f;

typedef struct __vkl_vec3f
{
  vkl_float x, y, z;
} vkl_vec3f;

typedef struct __vkl_vec4f
{
  vkl_float x, y, z, w;
} vkl_vec4f;

// -----------------------------------------------------------------------------
// VKL_DOUBLE
// -----------------------------------------------------------------------------

typedef double vkl_double;

typedef struct __vkl_vec2d
{
  vkl_double x, y;
} vkl_vec2d;

typedef struct __vkl_vec3d
{
  vkl_double x, y, z;
} vkl_vec3d;

typedef struct __vkl_vec4d
{
  vkl_double x, y, z, w;
} vkl_vec4d;

// -----------------------------------------------------------------------------
// VKL_BOX*I
// -----------------------------------------------------------------------------

typedef struct __vkl_box1i
{
  vkl_int lower, upper;
} vkl_box1i;

typedef struct __vkl_box2i
{
  vkl_vec2i lower, upper;
} vkl_box2i;

typedef struct __vkl_box3i
{
  vkl_vec3i lower, upper;
} vkl_box3i;

typedef struct __vkl_box4i
{
  vkl_vec4i lower, upper;
} vkl_box4i;

// -----------------------------------------------------------------------------
// VKL_BOX*F
// -----------------------------------------------------------------------------

typedef struct __vkl_box1f
{
  vkl_float lower, upper;
} vkl_box1f;

typedef struct __vkl_box2f
{
  vkl_vec2f lower, upper;
} vkl_box2f;

typedef struct __vkl_box3f
{
  vkl_vec3f lower, upper;
} vkl_box3f;

typedef struct __vkl_box4f
{
  vkl_vec4f lower, upper;
} vkl_box4f;

typedef vkl_box1f vkl_range1f;
typedef vkl_box2f vkl_range2f;
typedef vkl_box3f vkl_range3f;
typedef vkl_box4f vkl_range4f;

// -----------------------------------------------------------------------------
// VKL_LINEAR
// -----------------------------------------------------------------------------

typedef struct __vkl_linear2f
{
  vkl_vec2f vx, vy;
} vkl_linear2f;

typedef struct __vkl_linear3f
{
  vkl_vec3f vx, vy, vz;
} vkl_linear3f;

// -----------------------------------------------------------------------------
// VKL_AFFINE
// -----------------------------------------------------------------------------

typedef struct __vkl_affine2f
{
  vkl_linear2f l;
  vkl_vec2f p;
} vkl_affine2f;

typedef struct __vkl_affine3f
{
  vkl_linear3f l;
  vkl_vec3f p;
} vkl_affine3f;
