// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Sampler.h"
#include "../volume/Volume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    Sampler<W>::~Sampler()
    {
      assert(!ispcEquivalent); // Detect leaks in derived classes if possible.
    }

    template <int W>
    Observer<W> *Sampler<W>::newObserver(const char *type)
    {
      return nullptr;
    }

    template struct Sampler<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
