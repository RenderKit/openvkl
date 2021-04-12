// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ObserverRegistry.h"
#include "ObserverRegistry_ispc.h"
#include "../common/export_util.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    ObserverRegistry<W>::ObserverRegistry()
    {
      ispcEquivalent = CALL_ISPC(ObserverRegistry_create);
    }

    template <int W>
    ObserverRegistry<W>::~ObserverRegistry()
    {
      CALL_ISPC(ObserverRegistry_destroy, ispcEquivalent);
      ispcEquivalent = nullptr;
    }

    template <int W>
    void ObserverRegistry<W>::add(void *ptr)
    {
      std::lock_guard<std::recursive_mutex> g(mtx);
      CALL_ISPC(ObserverRegistry_add, ispcEquivalent, ptr);
    }

    template <int W>
    void ObserverRegistry<W>::remove(void *ptr)
    {
      std::lock_guard<std::recursive_mutex> g(mtx);
      CALL_ISPC(ObserverRegistry_remove, ispcEquivalent, ptr);
    }

    template struct ObserverRegistry<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
