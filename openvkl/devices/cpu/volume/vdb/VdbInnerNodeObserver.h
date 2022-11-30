// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/Allocator.h"
#include "../../observer/Observer.h"
#include "../../observer/ObserverRegistry.h"
#include "openvkl/ispc_cpp_interop.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    class VdbVolume;

    template <int W>
    struct VdbInnerNodeObserver : public Observer<W>
    {
      VdbInnerNodeObserver(VdbVolume<W> &target);

      VdbInnerNodeObserver(VdbInnerNodeObserver &&) = delete;
      VdbInnerNodeObserver &operator=(VdbInnerNodeObserver &&) = delete;
      VdbInnerNodeObserver(const VdbInnerNodeObserver &)       = delete;
      VdbInnerNodeObserver &operator=(const VdbInnerNodeObserver &) = delete;

      ~VdbInnerNodeObserver();

      const void *map() override;
      void unmap() override;
      VKLDataType getElementType() const override;
      size_t getElementSize() const override;
      size_t getNumElements() const override;

      void commit() override;

     private:
      void clear();

     private:
      Allocator allocator{this->getDevice()};
      size_t numFloats{0};
      size_t size{0};
      float *buffer{nullptr};
      using Observer<W>::target;
    };

  }  // namespace cpu_device
}  // namespace openvkl
