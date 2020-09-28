// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../iterator/DefaultIterator.h"
#include "../../iterator/Iterator.h"
#include "VdbGrid.h"
#include "VdbIterator_ispc.h"

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbIntervalIterator : public IntervalIterator<W>
    {
      using IntervalIterator<W>::IntervalIterator;

      void initializeIntervalV(
          const vintn<W> &valid,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override final;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override final;

      void *getIspcStorage() override final
      {
        return reinterpret_cast<void*>(ispcStorage);
      }

     protected:
      using Iterator<W>::sampler;
      using IspcIterator = __varying_ispc_type(VdbIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using VdbIntervalIteratorFactory =
        ConcreteIteratorFactory<W, IntervalIterator, VdbIntervalIterator>;

    template <int W>
    using VdbHitIterator = DefaultHitIterator<W, VdbIntervalIterator<W>>;

    template <int W>
    using VdbHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, VdbHitIterator>;

  }  // namespace ispc_driver
}  // namespace openvkl
