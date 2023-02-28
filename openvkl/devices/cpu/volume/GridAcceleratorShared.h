// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct SharedStructuredVolume;

  struct GridAccelerator
  {
    VKL_INTEROP_UNIFORM vec3i bricksPerDimension;
    VKL_INTEROP_UNIFORM size_t cellCount;
    box1f *VKL_INTEROP_UNIFORM cellValueRanges;
    SharedStructuredVolume *VKL_INTEROP_UNIFORM volume;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
