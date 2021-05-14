// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include <mutex>

namespace openvkl {
  namespace cpu_device {

    /*
     * Registry that can be used to keep track of observers that are relevant
     * for a given entity.
     *
     * This registry should be stored in the respective entity as a Ref<>
     * to ensure robust life time. Observers will call remove() when they are
     * destroyed.
     */
    template <int W>
    class ObserverRegistry
    {
     public:
      ObserverRegistry();
      ~ObserverRegistry();

      ObserverRegistry(const ObserverRegistry &) = delete;
      ObserverRegistry &operator=(const ObserverRegistry &) = delete;
      ObserverRegistry(ObserverRegistry &&other) = delete;
      ObserverRegistry &operator=(ObserverRegistry &&other) = delete;

      void add(void *ptr);

      void remove(void *ptr);

      inline void *getIE() {
        return ispcEquivalent;
      }

     private:
      void *ispcEquivalent{nullptr};
      std::recursive_mutex mtx;
    };

  }  // namespace cpu_device
}  // namespace openvkl
