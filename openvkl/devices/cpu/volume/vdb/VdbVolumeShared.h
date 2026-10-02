// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../VolumeShared.h"
#include "VdbGridShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  VKL_INTEROP_VARYING float VdbVolume_sample(
      const VdbGrid *VKL_INTEROP_UNIFORM grid,
      const VKL_INTEROP_VARYING vec3i &ic);

#ifdef __cplusplus
  typedef void *DenseLeafSamplingVaryingFunc;

  typedef void *DenseLeafSamplingUniformFunc;
#else
typedef varying float (*uniform DenseLeafSamplingVaryingFunc)(
    const VdbGrid *uniform grid,
    uniform uint32 attributeIndex,
    const varying vec3ui &offset,
    const varying float &time);

typedef uniform float (*uniform DenseLeafSamplingUniformFunc)(
    const VdbGrid *uniform grid,
    uniform uint32 attributeIndex,
    const uniform vec3ui &offset,
    uniform float time);
#endif

  struct VdbVolume
  {
    VolumeShared super;
    VdbGrid grid;  // embedded: samplers rely on a stable address

    // dense only, per attribute
    DenseLeafSamplingVaryingFunc *VKL_INTEROP_UNIFORM denseLeafSample_varying;
    DenseLeafSamplingUniformFunc *VKL_INTEROP_UNIFORM denseLeafSample_uniform;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
