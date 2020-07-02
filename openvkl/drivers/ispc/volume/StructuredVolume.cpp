// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "StructuredVolume.h"

#include "StructuredSampler.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    Sampler<W> *StructuredVolume<W>::newSampler()
    {
      return new StructuredSampler<W>(this);
    }

    template struct StructuredVolume<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
