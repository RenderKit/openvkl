// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../sampler/SamplerShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

#ifndef __ISPC_STRUCT_UnstructuredSamplerShared__
#define __ISPC_STRUCT_UnstructuredSamplerShared__
  struct UnstructuredSamplerShared
  {
    SamplerBaseShared super;
  };
#endif
#ifdef __cplusplus
}
#endif  // __cplusplus
