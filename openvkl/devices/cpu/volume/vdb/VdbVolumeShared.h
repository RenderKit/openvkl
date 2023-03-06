// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../VolumeShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  VKL_INTEROP_VARYING float VdbVolume_sample(
      const VdbGrid *VKL_INTEROP_UNIFORM grid,
      const VKL_INTEROP_VARYING vec3i &ic);

  struct VdbVolume
  {
    VolumeShared super;
    const VdbGrid *VKL_INTEROP_UNIFORM grid;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
