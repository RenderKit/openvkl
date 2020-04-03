// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Observer.h"
#include "openvkl/ispc_cpp_interop.h"

namespace openvkl {
  namespace ispc_driver {

    /*
     * The leaf access observer simply wraps the buffer allocated by VdbVolume.
     */
    struct VdbLeafAccessObserver : public Observer
    {
      VdbLeafAccessObserver(ManagedObject &target,
                            size_t size,
                            const vkl_uint32 *accessBuffer);

      VdbLeafAccessObserver(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver &operator=(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver(const VdbLeafAccessObserver &)       = delete;
      VdbLeafAccessObserver &operator=(const VdbLeafAccessObserver &) = delete;

      ~VdbLeafAccessObserver();

      const void *map() override;
      void unmap() override;
      VKLDataType getElementType() const override;
      size_t getNumElements() const override;

     private:
      ManagedObject *target{nullptr};
      size_t size{0};
      const vkl_uint32 *accessBuffer{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
