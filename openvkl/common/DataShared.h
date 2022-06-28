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
}  // namespace ispc
#endif  // __cplusplus

#endif
