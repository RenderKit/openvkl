// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct SamplerShared;

#ifdef __cplusplus

#ifndef __ISPC_STRUCT_box1f__
#define __ISPC_STRUCT_box1f__
  struct box1f
  {
    float lower;
    float upper;
  };
#endif

#ifndef __ISPC_STRUCT_ValueRanges__
#define __ISPC_STRUCT_ValueRanges__
  struct ValueRanges
  {
    int numRanges;
    box1f *ranges;
    box1f rangesMinMax;
  };
#endif

#else
#include "../devices/cpu/common/ValueRanges.ih"
#endif

#ifndef __ISPC_STRUCT_IteratorContext__
#define __ISPC_STRUCT_IteratorContext__
  struct IteratorContext
  {
    const SamplerShared *VKL_INTEROP_UNIFORM sampler;

    VKL_INTEROP_UNIFORM uint32 attributeIndex;

    VKL_INTEROP_UNIFORM ValueRanges valueRanges;

    VKL_INTEROP_UNIFORM uint32 maxIteratorDepth;
    VKL_INTEROP_UNIFORM bool elementaryCellIteration;
  };
#endif

#ifndef __ISPC_STRUCT_IntervalIteratorContext__
#define __ISPC_STRUCT_IntervalIteratorContext__
  struct IntervalIteratorContext
  {
    VKL_INTEROP_UNIFORM IteratorContext super;
  };
#endif

#ifndef __ISPC_STRUCT_HitIteratorContext__
#define __ISPC_STRUCT_HitIteratorContext__

  // hit iterator contexts inherit from interval contexts; this is because hit
  // iteration is often implemented using interval iteration, and we would like
  // to use the same context for that purpose.
  struct HitIteratorContext
  {
    VKL_INTEROP_UNIFORM IntervalIteratorContext super;

    VKL_INTEROP_UNIFORM int numValues;
    float *VKL_INTEROP_UNIFORM values;
  };

#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
