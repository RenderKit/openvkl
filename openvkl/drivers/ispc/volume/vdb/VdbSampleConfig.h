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
      VKLFilter filter VKL_INITIALIZER_LIST(VKL_FILTER_TRILINEAR);
      VKLFilter gradientFilter VKL_INITIALIZER_LIST(VKL_FILTER_TRILINEAR);
      vkl_uint32 maxSamplingDepth VKL_INITIALIZER_LIST(VKL_VDB_NUM_LEVELS - 1);

      void *leafAccessObservers VKL_INITIALIZER_LIST(nullptr);
    };

#if defined(__cplusplus)

  }  // namespace ispc_driver
}  // namespace openvkl

#endif  // defined(__cplusplus)
