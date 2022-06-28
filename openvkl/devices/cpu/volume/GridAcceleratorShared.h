// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/ispc_cpp_interop.h>

#ifdef __cplusplus
#include <rkcommon/math/box.h>
#endif  // __cplusplus

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct box1f;
  struct SharedStructuredVolume;

  struct GridAccelerator
  {
#ifdef __cplusplus
    VKL_INTEROP_UNIFORM rkcommon::math::vec3i bricksPerDimension;
#else
    VKL_INTEROP_UNIFORM vec3i bricksPerDimension;
#endif
    VKL_INTEROP_UNIFORM int spacer; //enforce alignment between cpp and ispc
    VKL_INTEROP_UNIFORM size_t cellCount;
    box1f *VKL_INTEROP_UNIFORM cellValueRanges;
    SharedStructuredVolume *VKL_INTEROP_UNIFORM volume;
  };
#ifdef __cplusplus
}
#endif  // __cplusplus
