// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/export_util.h"
#include "../iterator/DefaultIterator.h"
#include "../iterator/GridAcceleratorIterator.h"
#include "../sampler/Sampler.h"
#include "Sampler_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredRegularVolume.h"
#include "StructuredSphericalVolume.h"
#include "StructuredVolume.h"
#include "Volume_ispc.h"
#include "openvkl/VKLFilter.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    struct StructuredSampler : public SamplerBase<W,
                                                  StructuredVolume,
                                                  IntervalIteratorFactory,
                                                  HitIteratorFactory>
    {
      StructuredSampler(StructuredVolume<W> *volume);
      ~StructuredSampler() override;

      void commit() override;

      // single attribute /////////////////////////////////////////////////////

      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples,
                         unsigned int attributeIndex,
                         const vfloatn<1> &time) const override final;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples,
                          unsigned int attributeIndex,
                          const vfloatn<W> &time) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples,
                          unsigned int attributeIndex,
                          const float *times) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients,
                            unsigned int attributeIndex,
                            const vfloatn<W> &time) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients,
                            unsigned int attributeIndex,
                            const float *times) const override final;

      // multi-attribute //////////////////////////////////////////////////////

      void computeSampleM(const vvec3fn<1> &objectCoordinates,
                          float *samples,
                          unsigned int M,
                          const unsigned int *attributeIndices,
                          const vfloatn<1> &time) const override final;

      void computeSampleMV(const vintn<W> &valid,
                           const vvec3fn<W> &objectCoordinates,
                           float *samples,
                           unsigned int M,
                           const unsigned int *attributeIndices,
                           const vfloatn<W> &time) const override final;

      void computeSampleMN(unsigned int N,
                           const vvec3fn<1> *objectCoordinates,
                           float *samples,
                           unsigned int M,
                           const unsigned int *attributeIndices,
                           const float *times) const override final;

      /////////////////////////////////////////////////////////////////////////

     protected:
      using Sampler<W>::ispcEquivalent;
      using SamplerBase<W,
                        StructuredVolume,
                        IntervalIteratorFactory,
                        HitIteratorFactory>::volume;

      VKLFilter filter;
      VKLFilter gradientFilter;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        StructuredSampler(StructuredVolume<W> *volume)
        : SamplerBase<W,
                      StructuredVolume,
                      IntervalIteratorFactory,
                      HitIteratorFactory>(*volume),
          filter(volume->getFilter()),
          gradientFilter(volume->getGradientFilter())
    {
      assert(volume);
      ispcEquivalent =
          CALL_ISPC(StructuredSampler_create, volume->getISPCEquivalent());
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        ~StructuredSampler()
    {
      CALL_ISPC(StructuredSampler_destroy, ispcEquivalent);
      ispcEquivalent = nullptr;
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::commit()
    {
      filter = (VKLFilter)this->template getParam<int>("filter", filter);

      // Note: We fall back to the sampler object filter parameter if it is set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly
      //       if they set filter and want gradientFilter to be different.
      gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter", this->hasParam("filter") ? filter : gradientFilter);

      CALL_ISPC(Sampler_setFilters,
                ispcEquivalent,
                (ispc::VKLFilter)filter,
                (ispc::VKLFilter)gradientFilter);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSample(const vvec3fn<1> &objectCoordinates,
                      vfloatn<1> &samples,
                      unsigned int attributeIndex,
                      const vfloatn<1> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(SharedStructuredVolume_sample_uniform_export,
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                &time,
                &samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSampleV(const vintn<W> &valid,
                       const vvec3fn<W> &objectCoordinates,
                       vfloatn<W> &samples,
                       unsigned int attributeIndex,
                       const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(SharedStructuredVolume_sample_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                &time,
                &samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSampleN(unsigned int N,
                       const vvec3fn<1> *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex,
                       const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_sample_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                attributeIndex,
                times,
                samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeGradientV(const vintn<W> &valid,
                         const vvec3fn<W> &objectCoordinates,
                         vvec3fn<W> &gradients,
                         unsigned int attributeIndex,
                         const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(SharedStructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                time,
                &gradients);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeGradientN(unsigned int N,
                         const vvec3fn<1> *objectCoordinates,
                         vvec3fn<1> *gradients,
                         unsigned int attributeIndex,
                         const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_gradient_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                attributeIndex,
                times,
                (ispc::vec3f *)gradients);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSampleM(const vvec3fn<1> &objectCoordinates,
                       float *samples,
                       unsigned int M,
                       const unsigned int *attributeIndices,
                       const vfloatn<1> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTimes(time);
      CALL_ISPC(SharedStructuredVolume_sampleM_uniform_export,
                ispcEquivalent,
                &objectCoordinates,
                M,
                attributeIndices,
                &time,
                samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSampleMV(const vintn<W> &valid,
                        const vvec3fn<W> &objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const vfloatn<W> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTimes(time);
      CALL_ISPC(SharedStructuredVolume_sampleM_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                M,
                attributeIndices,
                &time,
                samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory>
    inline void
    StructuredSampler<W, IntervalIteratorFactory, HitIteratorFactory>::
        computeSampleMN(unsigned int N,
                        const vvec3fn<1> *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_sampleM_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                M,
                attributeIndices,
                times,
                samples);
    }

    template <int W>
    using StructuredRegularSampler =
        StructuredSampler<W,
                          GridAcceleratorIntervalIteratorFactory,
                          GridAcceleratorHitIteratorFactory>;

    template <int W>
    using StructuredSphericalIntervalIteratorFactory =
        ConcreteIteratorFactory<W, IntervalIterator, DefaultIntervalIterator>;

    template <int W>
    using StructuredSphericalHitIterator =
        DefaultHitIterator<W, DefaultIntervalIterator<W>>;

    template <int W>
    using StructuredSphericalHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, StructuredSphericalHitIterator>;

    template <int W>
    using StructuredSphericalSampler =
        StructuredSampler<W,
                          StructuredSphericalIntervalIteratorFactory,
                          StructuredSphericalHitIteratorFactory>;

  }  // namespace ispc_driver
}  // namespace openvkl
