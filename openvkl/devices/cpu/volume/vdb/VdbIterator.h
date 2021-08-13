// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../iterator/DefaultIterator.h"
#include "../../iterator/Iterator.h"
#include "../../iterator/IteratorContext.h"
#include "VdbGrid.h"
#include "VdbIterator_ispc.h"

using namespace rkcommon;

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct VdbIntervalIterator : public IntervalIterator<W>
    {
      using IntervalIterator<W>::IntervalIterator;

      void initializeIntervalV(
          const vintn<W> &valid,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const vfloatn<W> &times) override final;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override final;

      void *getIspcStorage() override final
      {
        return reinterpret_cast<void*>(ispcStorage);
      }

     protected:
      using Iterator<W>::context;
      using IspcIterator = __varying_ispc_type(VdbIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using VdbIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                VdbIntervalIterator,
                                IntervalIteratorContext,
                                IntervalIteratorContext>;

    template <int W>
    using VdbHitIterator = DefaultHitIterator<W, VdbIntervalIterator<W>>;

    template <int W>
    using VdbHitIteratorFactory = ConcreteIteratorFactory<W,
                                                          HitIterator,
                                                          VdbHitIterator,
                                                          HitIteratorContext,
                                                          HitIteratorContext>;

  }  // namespace cpu_device
}  // namespace openvkl
