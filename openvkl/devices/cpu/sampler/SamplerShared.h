// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/VKLFilter.h"
#include "openvkl/ispc_cpp_interop.h"
#include "../volume/VolumeShared.h"

#ifdef __cplusplus
namespace ispc {
#else
#include "rkcommon/math/vec.ih"
#endif  // __cplusplus

#ifndef __ISPC_STRUCT_SamplerShared__
#define __ISPC_STRUCT_SamplerShared__

  struct SamplerShared
  {
    const VolumeShared *VKL_INTEROP_UNIFORM volume;

    // these functions do not yet support multi-attribute volumes. they are used
    // in generic stream-wide sampling and gradient implementations, as well as
    // in hit iterator surface intersection functions (which today only support
    // the first attribute).

#ifdef __cplusplus
    void *computeSample_uniform;

    void *computeSample_varying;

    void *computeGradient_varying;
#else
    uniform float (*uniform computeSample_uniform)(
        const SamplerShared *uniform _self,
        const uniform vec3f &objectCoordinates,
        const uniform uint32 attributeIndex,
        const uniform float &time);

    varying float (*uniform computeSample_varying)(
        const SamplerShared *uniform _self,
        const varying vec3f &objectCoordinates,
        const uniform uint32 attributeIndex,
        const varying float &time);

    varying vec3f (*uniform computeGradient_varying)(
        const SamplerShared *uniform _self,
        const varying vec3f &objectCoordinates);
#endif

    // Samplers may choose to implement these filter modes.
    VKLFilter filter;
    VKLFilter gradientFilter;
  };

#endif

#ifndef __ISPC_STRUCT_SamplerBaseShared__
#define __ISPC_STRUCT_SamplerBaseShared__
  struct SamplerBaseShared
  {
    SamplerShared super;
  };
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
