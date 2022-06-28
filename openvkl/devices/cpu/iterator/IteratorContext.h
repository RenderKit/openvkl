// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "openvkl/common/BufferShared.h"
#include "openvkl/common/StructShared.h"
#include "IteratorContextShared.h"

using namespace rkcommon::math;

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct Sampler;

    ///////////////////////////////////////////////////////////////////////////
    // Iterator context base class ////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct IteratorContext
        : public AddStructShared<ManagedObject, ispc::IteratorContext>
    {
      IteratorContext(const Sampler<W> &sampler);

      virtual ~IteratorContext();

      virtual void commit() = 0;

      const Sampler<W> &getSampler() const;

     protected:
      Ref<const Sampler<W>> sampler;
      int attributeIndex = 0;
      bool SharedStructInitialized = false;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline IteratorContext<W>::IteratorContext(const Sampler<W> &sampler)
        : sampler(&sampler)
    {
    }

    template <int W>
    inline IteratorContext<W>::~IteratorContext()
    {
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
    struct IntervalIteratorContext
        : public AddStructShared<IteratorContext<W>,
                                 ispc::IntervalIteratorContext>
    {
      IntervalIteratorContext(const Sampler<W> &sampler)
          : AddStructShared<IteratorContext<W>, ispc::IntervalIteratorContext>(
                sampler)
      {
      }
      virtual ~IntervalIteratorContext();

      void commit() override;
    protected:
      std::unique_ptr<BufferShared<range1f>> rangesView;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator context ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // hit iterator contexts inherit from interval contexts; this is because hit
    // iteration is often implemented using interval iteration, and we would
    // like to use the same context for that purpose.
    template <int W>
    struct HitIteratorContext
        : public AddStructShared<IntervalIteratorContext<W>,
                                 ispc::HitIteratorContext>
    {
      HitIteratorContext(const Sampler<W> &sampler)
          : AddStructShared<IntervalIteratorContext<W>,
                            ispc::HitIteratorContext>(sampler)
      {
      }

      virtual ~HitIteratorContext();

      void commit() override;
    private:
      std::unique_ptr<BufferShared<float>> valuesView;
    };

  }  // namespace cpu_device
}  // namespace openvkl
