// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GridAcceleratorIteratorSize_ispc.h"
#include "Iterator.h"
#include "IteratorContext.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct GridAcceleratorIntervalIterator : public IntervalIterator<W>
    {

      explicit GridAcceleratorIntervalIterator(const IntervalIteratorContext<W> &context)
            : IntervalIterator<W>(context)
      {
        this->kernelStorage = (void*)ispcStorage;
      }

      using IntervalIterator<W>::IntervalIterator;

      // Varying.

      void initializeIntervalV(const vintn<W> &valid,
                               const vvec3fn<W> &origin,
                               const vvec3fn<W> &direction,
                               const vrange1fn<W> &tRange,
                               const vfloatn<W> &times) override final;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override final;

      // Uniform.

      void initializeIntervalU(const vvec3fn<1> &origin,
                               const vvec3fn<1> &direction,
                               const vrange1fn<1> &tRange,
                               float time) override final;

      void iterateIntervalU(vVKLIntervalN<1> &interval,
                            vintn<1> &result) override final;

      void *getIspcStorage() override final
      {
        return reinterpret_cast<void *>(ispcStorage);
      }

     protected:
      using Iterator<W>::context;
      using IspcIterator = __varying_ispc_type(GridAcceleratorIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using GridAcceleratorIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                GridAcceleratorIntervalIterator,
                                IntervalIteratorContext,
                                IntervalIteratorContext>;

    template <int W>
    struct GridAcceleratorHitIterator : public HitIterator<W>
    {
      using HitIterator<W>::HitIterator;

      // Varying.

      void initializeHitV(const vintn<W> &valid,
                          const vvec3fn<W> &origin,
                          const vvec3fn<W> &direction,
                          const vrange1fn<W> &tRange,
                          const vfloatn<W> &times) override final;
      void iterateHitV(const vintn<W> &valid,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override final;

      // Uniform.

      void initializeHitU(const vvec3fn<1> &origin,
                          const vvec3fn<1> &direction,
                          const vrange1fn<1> &tRange,
                          float time) override final;

      void iterateHitU(vVKLHitN<1> &hit, vintn<1> &result) override final;

     protected:
      using Iterator<W>::context;
      using IspcIterator = __varying_ispc_type(GridAcceleratorIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using GridAcceleratorHitIteratorFactory =
        ConcreteIteratorFactory<W,
                                HitIterator,
                                GridAcceleratorHitIterator,
                                HitIteratorContext,
                                HitIteratorContext>;

  }  // namespace cpu_device
}  // namespace openvkl
