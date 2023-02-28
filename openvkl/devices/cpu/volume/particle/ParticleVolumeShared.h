// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UnstructuredVolumeBaseShared.h"
#include "../common/DataShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct VKLParticleVolume
  {
    VKLUnstructuredBase super;

    VKL_INTEROP_UNIFORM float clampMaxCumulativeValue;
    VKL_INTEROP_UNIFORM float radiusSupportFactor;
    VKL_INTEROP_UNIFORM Data1D positions;
    VKL_INTEROP_UNIFORM Data1D radii;
    VKL_INTEROP_UNIFORM Data1D weights;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
