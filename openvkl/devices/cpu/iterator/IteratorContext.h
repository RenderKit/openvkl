// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "rkcommon/math/range.h"

using namespace rkcommon::math;

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

      virtual void commit() = 0;

      void *getISPCEquivalent() const;

      const Sampler<W> &getSampler() const;

     protected:
      Ref<const Sampler<W>> sampler;
      const unsigned int attributeIndex;

      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline IteratorContext<W>::IteratorContext(const Sampler<W> &sampler,
                                               unsigned int attributeIndex)
        : sampler(&sampler), attributeIndex(attributeIndex)
    {
    }

    template <int W>
    inline IteratorContext<W>::~IteratorContext()
    {
      assert(!ispcEquivalent);  // Detect leaks in derived classes if possible.
    }

    template <int W>
    inline void *IteratorContext<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

    template <int W>
    inline const Sampler<W> &IteratorContext<W>::getSampler() const
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
                              unsigned int attributeIndex)
          : IteratorContext<W>(sampler, attributeIndex)
      {
      }

      virtual ~IntervalIteratorContext();

      void commit() override;

     private:
      std::vector<range1f> valueRanges;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator context ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // hit iterator contexts inherit from interval contexts; this is because hit
    // iteration is often implemented using interval iteration, and we would
    // like to use the same context for that purpose.
    template <int W>
    struct HitIteratorContext : public IntervalIteratorContext<W>
    {
      HitIteratorContext(const Sampler<W> &sampler, unsigned int attributeIndex)
          : IntervalIteratorContext<W>(sampler, attributeIndex)
      {
      }

      virtual ~HitIteratorContext();

      void commit() override;

     private:
      std::vector<float> values;
    };

  }  // namespace cpu_device
}  // namespace openvkl
