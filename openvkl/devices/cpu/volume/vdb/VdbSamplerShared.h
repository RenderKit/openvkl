// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#include "../../sampler/SamplerShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

#ifndef __ISPC_STRUCT_VdbSamplerShared__
#define __ISPC_STRUCT_VdbSamplerShared__

  struct VdbSamplerShared
  {
    SamplerBaseShared super;

    const VdbGrid *VKL_INTEROP_UNIFORM grid;
    const void *VKL_INTEROP_UNIFORM leafAccessObservers;
    vkl_uint32 maxSamplingDepth;
  };

#endif
#ifdef __cplusplus
}
#endif  // __cplusplus
