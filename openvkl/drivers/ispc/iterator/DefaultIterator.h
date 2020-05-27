// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Iterator.h"
#include "DefaultIterator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct DefaultIntervalIterator : public IntervalIterator<W>
    {
      using IntervalIterator<W>::IntervalIterator;

      void initializeIntervalV(
          const vintn<W> &valid,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override final;

      void iterateIntervalV(const vintn<W> &valid,
                            Interval<W> &interval,
                            vintn<W> &result) override final;

     protected:
      using Iterator<W>::volume;
      using IspcIterator = __varying_ispc_type(DefaultIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using DefaultIntervalIteratorFactory =
        ConcreteIteratorFactory<W, IntervalIterator, DefaultIntervalIterator>;

    ////////////////////////////////////////////////////////////////////////////

    template <int W>
    struct DefaultHitIterator : public HitIterator<W>
    {
      using HitIterator<W>::HitIterator;

      void initializeHitV(const vintn<W> &valid,
                          const vvec3fn<W> &origin,
                          const vvec3fn<W> &direction,
                          const vrange1fn<W> &tRange,
                          const ValueSelector<W> *valueSelector) override final;

      void iterateHitV(const vintn<W> &valid,
                       Hit<W> &hit,
                       vintn<W> &result) override final;

     protected:
      using Iterator<W>::volume;
      using IspcIterator = __varying_ispc_type(DefaultIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using DefaultHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, DefaultHitIterator>;

  }  // namespace ispc_driver
}  // namespace openvkl
