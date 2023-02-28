// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "StructuredVolume.h"

#include "StructuredSampler.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    Sampler<W> *StructuredVolume<W>::newSampler()
    {
      return new StructuredRegularSampler<W>(*this);
    }

    template struct StructuredVolume<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
