// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/export_util.h"
#include "DefaultIterator_ispc.h"
#include "Iterator.h"

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
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override final;

      void *getIspcStorage() override final
      {
        return reinterpret_cast<void *>(ispcStorage);
      }

     protected:
      using Iterator<W>::sampler;
      using IspcIterator = __varying_ispc_type(DefaultIntervalIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    ////////////////////////////////////////////////////////////////////////////

    template <int W, class IntervalIterator>
    struct DefaultHitIterator : public HitIterator<W>
    {
      explicit DefaultHitIterator(const Sampler<W> &sampler);

      void initializeHitV(const vintn<W> &valid,
                          const vvec3fn<W> &origin,
                          const vvec3fn<W> &direction,
                          const vrange1fn<W> &tRange,
                          const vfloatn<W> &times,
                          const ValueSelector<W> *valueSelector) override final;

      void iterateHitV(const vintn<W> &valid,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override final;

     protected:
      IntervalIterator intervalIterator;

      using Iterator<W>::sampler;
      using IspcIterator = __varying_ispc_type(DefaultHitIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    ////////////////////////////////////////////////////////////////////////////
    // Implementation for the default hit iterator must be in the header
    // to facilitate easy usage for volume-specific hit iterators.
    ////////////////////////////////////////////////////////////////////////////

    template <int W, class IntervalIterator>
    DefaultHitIterator<W, IntervalIterator>::DefaultHitIterator(
        const Sampler<W> &sampler)
        : HitIterator<W>(sampler), intervalIterator(sampler)
    {
    }

    template <int W, class IntervalIterator>
    void DefaultHitIterator<W, IntervalIterator>::initializeHitV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const vfloatn<W> &times,
        const ValueSelector<W> *valueSelector)
    {
      assertValidTimes(times);

      intervalIterator.initializeIntervalV(valid,
                                           origin,
                                           direction,
                                           tRange,
                                           valueSelector);

      CALL_ISPC(DefaultHitIterator_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                intervalIterator.getIspcStorage(),
                sampler->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&times,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W, class IntervalIterator>
    void DefaultHitIterator<W, IntervalIterator>::iterateHitV(
        const vintn<W> &valid, vVKLHitN<W> &hit, vintn<W> &result)
    {
      CALL_ISPC(DefaultHitIterator_iterateHit,
                static_cast<const int *>(valid),
                ispcStorage,
                &hit,
                static_cast<int *>(result));
    }

  }  // namespace ispc_driver
}  // namespace openvkl
