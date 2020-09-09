// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Sampler.h"
#include "../volume/Volume.h"

namespace openvkl {
  namespace ispc_driver {

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

    template <int W>
    void *Sampler<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

    template struct Sampler<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
