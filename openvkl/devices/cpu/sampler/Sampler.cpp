// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../volume/Volume.h"
#include "Sampler.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    Sampler<W>::~Sampler()
    {
    }

    template <int W>
    Observer<W> *Sampler<W>::newObserver(const char *type)
    {
      return nullptr;
    }

    template struct Sampler<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
