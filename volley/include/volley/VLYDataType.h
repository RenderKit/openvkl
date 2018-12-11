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

#include <cstdint>

// An enum type that represensts the different data types represented in volley
//
// IMPORTANT: enums added here should also be represented in stringForType() and
// other functions in VLYCommon.cpp!
typedef enum
# if __cplusplus >= 201103L
: uint32_t
#endif
{
  //! Driver reference type.
  VLY_DRIVER = 100,

  //! Void pointer type.
  VLY_VOID_PTR = 200,

  //! Object reference type.
  VLY_OBJECT = 1000,

  //! Object reference subtypes.
  VLY_DATA,
  VLY_TRANSFER_FUNCTION,
  VLY_VOLUME,

  //! Pointer to a C-style NULL-terminated character string.
  VLY_STRING = 1500,

  //! Character scalar type.
  VLY_CHAR = 2000,

  //! Unsigned character scalar and vector types.
  VLY_UCHAR = 2500, VLY_UCHAR2, VLY_UCHAR3, VLY_UCHAR4,

  //! Signed 16-bit integer scalar.
  VLY_SHORT = 3000,

  //! Unsigned 16-bit integer scalar.
  VLY_USHORT = 3500,

  //! Signed 32-bit integer scalar and vector types.
  VLY_INT = 4000, VLY_INT2, VLY_INT3, VLY_INT4,

  //! Unsigned 32-bit integer scalar and vector types.
  VLY_UINT = 4500, VLY_UINT2, VLY_UINT3, VLY_UINT4,

  //! Signed 64-bit integer scalar and vector types.
  VLY_LONG = 5000, VLY_LONG2, VLY_LONG3, VLY_LONG4,

  //! Unsigned 64-bit integer scalar and vector types.
  VLY_ULONG = 5550, VLY_ULONG2, VLY_ULONG3, VLY_ULONG4,

  //! Single precision floating point scalar and vector types.
  VLY_FLOAT = 6000, VLY_FLOAT2, VLY_FLOAT3, VLY_FLOAT4, VLY_FLOAT3A,

  //! Double precision floating point scalar type.
  VLY_DOUBLE = 7000,

  //! Guard value.
  VLY_UNKNOWN = 22222,

} VLYDataType;
