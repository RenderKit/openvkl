// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/math.h"
#include "DefaultIterator.h"
#include "Iterator.h"
#include "IteratorContext.h"
#include "UnstructuredIterator_ispc.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct UnstructuredIntervalIterator : public IntervalIterator<W>
    {
      using IntervalIterator<W>::IntervalIterator;

      void initializeIntervalV(
          const vintn<W> &valid,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange) override final;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override final;

      void *getIspcStorage() override final
      {
        return reinterpret_cast<void*>(ispcStorage);
      }

     protected:
      using Iterator<W>::context;
      using IspcIterator = __varying_ispc_type(UnstructuredIterator);
      alignas(alignof(IspcIterator)) char ispcStorage[sizeof(IspcIterator)];
    };

    template <int W>
    using UnstructuredIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                UnstructuredIntervalIterator,
                                IntervalIteratorContext,
                                IntervalIteratorContext>;

    template <int W>
    using UnstructuredHitIterator = DefaultHitIterator<W, UnstructuredIntervalIterator<W>>;

    template <int W>
    using UnstructuredHitIteratorFactory =
        ConcreteIteratorFactory<W,
                                HitIterator,
                                UnstructuredHitIterator,
                                HitIteratorContext,
                                HitIteratorContext>;

  }  // namespace cpu_device
}  // namespace openvkl
