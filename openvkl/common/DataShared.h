// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifndef __ISPC_STRUCT_Data1D__
#define __ISPC_STRUCT_Data1D__

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct Data1D
  {
    const vkl_uint8 *addr;
    vkl_uint64 byteStride;
    vkl_uint64 numItems;
#ifdef __cplusplus
    VKLDataType dataType;
#else
    vkl_uint32 dataType;
#endif
    bool compact;
  };

#ifdef __cplusplus
#ifndef __ISPC_STRUCT_vec3i__
#define __ISPC_STRUCT_vec3i__
struct vec3i {
    int32_t x;
    int32_t y;
    int32_t z;
};
#endif
#ifndef __ISPC_STRUCT_vec3f__
#define __ISPC_STRUCT_vec3f__
struct vec3f {
    float x;
    float y;
    float z;
};
#endif

#ifndef __ISPC_STRUCT_vec4f__
#define __ISPC_STRUCT_vec4f__
struct vec4f {
    float x;
    float y;
    float z;
    float w;
};
#endif

#ifndef __ISPC_STRUCT_box3f__
#define __ISPC_STRUCT_box3f__
struct box3f {
    struct vec3f lower;
    struct vec3f upper;
};
#endif

#ifndef __ISPC_STRUCT_Data1D__
#define __ISPC_STRUCT_Data1D__
struct Data1D {
    const uint8_t * addr;
    uint64_t byteStride;
    uint64_t numItems;
    uint32_t dataType;
    bool compact;
};
#endif

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
#endif

#ifdef __cplusplus
}  // namespace ispc
#endif  // __cplusplus

#endif
