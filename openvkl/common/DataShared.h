// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

#ifndef __ISPC_STRUCT_Data1D__
#define __ISPC_STRUCT_Data1D__
  struct Data1D
  {
    const vkl_uint8 *addr;
    vkl_uint64 byteStride;
    vkl_uint64 numItems;
    vkl_uint32 dataType;
    bool compact;
  };
#endif

#ifdef __cplusplus
}  // namespace ispc
#endif  // __cplusplus
