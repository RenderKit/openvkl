// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../sampler/SamplerShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct StructuredSamplerShared
  {
    SamplerBaseShared super;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
