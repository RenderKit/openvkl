// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct Sampler;

    ///////////////////////////////////////////////////////////////////////////
    // Iterator context base class ////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct IteratorContext : public ManagedObject
    {
      IteratorContext(const Sampler<W> &sampler, unsigned int attributeIndex);

      virtual ~IteratorContext();

      void *getISPCEquivalent() const;

      Sampler<W> &getSampler();
      const Sampler<W> &getSampler() const;

     protected:
      Ref<const Sampler<W>> sampler;
      const unsigned int attributeIndex;

      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    IteratorContext<W>::IteratorContext(const Sampler<W> &sampler,
                                        unsigned int attributeIndex)
        : sampler(&sampler), attributeIndex(attributeIndex)
    {
    }

    template <int W>
    IteratorContext<W>::~IteratorContext()
    {
      assert(!ispcEquivalent);  // Detect leaks in derived classes if possible.
    }

    template <int W>
    inline void *IteratorContext<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

    template <int W>
    inline Sampler<W> &IteratorContext<W>::getSampler()
    {
      return *sampler;
    }

    template <int W>
    const Sampler<W> &IteratorContext<W>::getSampler() const
    {
      return *sampler;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator context //////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct IntervalIteratorContext : public IteratorContext<W>
    {
      IntervalIteratorContext(const Sampler<W> &sampler,
                              unsigned int attributeIndex);

      virtual ~IntervalIteratorContext();

      void commit() override {}
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    IntervalIteratorContext<W>::IntervalIteratorContext(
        const Sampler<W> &sampler, unsigned int attributeIndex)
        : IteratorContext<W>(sampler, attributeIndex)
    {
    }

    template <int W>
    IntervalIteratorContext<W>::~IntervalIteratorContext()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator context ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct HitIteratorContext : public IteratorContext<W>
    {
      HitIteratorContext(const Sampler<W> &sampler,
                         unsigned int attributeIndex);
      virtual ~HitIteratorContext();

      void commit() override {}
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    HitIteratorContext<W>::HitIteratorContext(const Sampler<W> &sampler,
                                              unsigned int attributeIndex)
        : IteratorContext<W>(sampler, attributeIndex)
    {
    }

    template <int W>
    HitIteratorContext<W>::~HitIteratorContext()
    {
    }

  }  // namespace cpu_device
}  // namespace openvkl
