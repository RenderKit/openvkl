// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/VKLFilter.h"
#include "openvkl/ispc_cpp_interop.h"

#if defined(__cplusplus)

namespace openvkl {
  namespace ispc_driver {

#endif  // defined(__cplusplus)

    struct VdbSampleConfig
    {
      VKLFilter filter;
      vkl_uint32 maxSamplingDepth;
    };

#if defined(__cplusplus)

  }  // namespace ispc_driver
}  // namespace openvkl

#endif  // defined(__cplusplus)
