// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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

  // Object reference type.
  VKL_OBJECT = 1000,

  // Object reference subtypes.
  VKL_DATA,
  VKL_SAMPLES_MASK,
  VKL_VOLUME,

  // Pointer to a C-style NULL-terminated character string.
  VKL_STRING = 1500,

  // Character scalar type.
  VKL_CHAR = 2000,

  // Unsigned character scalar and vector types.
  VKL_UCHAR = 2500, VKL_UCHAR2, VKL_UCHAR3, VKL_UCHAR4,

  // Signed 16-bit integer scalar.
  VKL_SHORT = 3000,

  // Unsigned 16-bit integer scalar.
  VKL_USHORT = 3500,

  // Signed 32-bit integer scalar and vector types.
  VKL_INT = 4000, VKL_INT2, VKL_INT3, VKL_INT4,

  // Unsigned 32-bit integer scalar and vector types.
  VKL_UINT = 4500, VKL_UINT2, VKL_UINT3, VKL_UINT4,

  // Signed 64-bit integer scalar and vector types.
  VKL_LONG = 5000, VKL_LONG2, VKL_LONG3, VKL_LONG4,

  // Unsigned 64-bit integer scalar and vector types.
  VKL_ULONG = 5550, VKL_ULONG2, VKL_ULONG3, VKL_ULONG4,

  // Single precision floating point scalar and vector types.
  VKL_FLOAT = 6000, VKL_FLOAT2, VKL_FLOAT3, VKL_FLOAT4, VKL_FLOAT3A,

  // Double precision floating point scalar type.
  VKL_DOUBLE = 7000,

  // Guard value.
  VKL_UNKNOWN = 22222,

} VKLDataType;
