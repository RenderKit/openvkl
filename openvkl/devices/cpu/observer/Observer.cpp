// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "Observer.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    Observer<W>::Observer(ManagedObject &target)
        : ManagedObject(target.getDevice()), target(&target)
    {
    }

    template <int W>
    Observer<W>::~Observer()
    {
    }

    template <int W>
    std::string Observer<W>::toString() const
    {
      return "openvkl::Observer";
    }

    template struct Observer<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
