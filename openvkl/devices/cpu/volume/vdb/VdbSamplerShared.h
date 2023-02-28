// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbGrid.h"

#include "openvkl/ispc_cpp_interop.h"

#include "../../sampler/SamplerShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

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

#ifndef __ISPC_STRUCT_VdbSamplerShared__
#define __ISPC_STRUCT_VdbSamplerShared__

  struct VdbSamplerShared
  {
    SamplerBaseShared super;

    const VdbGrid *VKL_INTEROP_UNIFORM grid;
    const void *VKL_INTEROP_UNIFORM leafAccessObservers;
    vkl_uint32 maxSamplingDepth;

    DenseLeafSamplingVaryingFunc *VKL_INTEROP_UNIFORM denseLeafSample_varying;
    DenseLeafSamplingUniformFunc *VKL_INTEROP_UNIFORM denseLeafSample_uniform;
  };

#endif
#ifdef __cplusplus
}
#endif  // __cplusplus
