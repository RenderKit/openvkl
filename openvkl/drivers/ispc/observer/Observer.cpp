// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Observer.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    Observer<W>::Observer(ManagedObject &target) : target(&target)
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

  }  // namespace ispc_driver
}  // namespace openvkl
