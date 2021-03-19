// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/Allocator.h"
#include "../../observer/Observer.h"
#include "../../observer/ObserverRegistry.h"
#include "openvkl/ispc_cpp_interop.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    class VdbSampler;
    struct VdbGrid;

    /*
     * The leaf access observer simply wraps the buffer allocated by VdbVolume.
     */
    template <int W>
    struct VdbLeafAccessObserver : public Observer<W>
    {
      VdbLeafAccessObserver(VdbSampler<W> &target, const VdbGrid &grid);

      VdbLeafAccessObserver(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver &operator=(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver(const VdbLeafAccessObserver &)       = delete;
      VdbLeafAccessObserver &operator=(const VdbLeafAccessObserver &) = delete;

      ~VdbLeafAccessObserver();

      const void *map() override;
      void unmap() override;
      VKLDataType getElementType() const override;
      size_t getElementSize() const override;
      size_t getNumElements() const override;

     private:
      ObserverRegistry<W> &getRegistry();

     private:
      Allocator allocator;
      size_t size{0};
      vkl_uint32 *accessBuffer{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
