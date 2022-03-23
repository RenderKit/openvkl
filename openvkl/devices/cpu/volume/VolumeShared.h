// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

// this hack prevents .ispc -> ispc.h code from redeclaring. TODO: something
// better
#ifndef __ISPC_STRUCT_VolumeShared__
#define __ISPC_STRUCT_VolumeShared__

  struct VolumeShared
  {
    // Background value, per attribute.
    const float *VKL_INTEROP_UNIFORM background;
  };

#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
