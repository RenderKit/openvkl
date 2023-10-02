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
#include "StructuredSamplerShared.h"
#include "StructuredSphericalVolume.h"
#include "StructuredVolume.h"
#include "openvkl/VKLFilter.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    struct StructuredSampler
        : public AddStructShared<SamplerBase<W,
                                             StructuredVolume,
                                             IntervalIteratorFactory,
                                             HitIteratorFactory>,
                                 ispc::StructuredSamplerShared>
    {
      StructuredSampler(Device *device, StructuredVolume<W> &volume);
      ~StructuredSampler() override;

      void commit() override;

      VKLFeatureFlagsInternal getFeatureFlags() const override;

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
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::StructuredSampler(Device *device,
                                         StructuredVolume<W> &volume)
        : AddStructShared<SamplerBase<W,
                                      StructuredVolume,
                                      IntervalIteratorFactory,
                                      HitIteratorFactory>,
                          ispc::StructuredSamplerShared>(device, volume),
          filter(volume.getFilter()),
          gradientFilter(volume.getGradientFilter())
    {
      ispc::StructuredSamplerShared *ssampler =
          (ispc::StructuredSamplerShared *)this->getSh();
      memset(ssampler, 0, sizeof(ispc::StructuredSamplerShared));

      ispc::SamplerShared *sampler = &ssampler->super.super;
      sampler->volume              = (const ispc::VolumeShared *)volume.getSh();
      CALL_ISPC(assignStructuredSamplerKernels, this->getSh());
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline StructuredSampler<W,
                             IntervalIteratorFactory,
                             HitIteratorFactory,
                             featureFlags>::~StructuredSampler()
    {
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<W,
                                  IntervalIteratorFactory,
                                  HitIteratorFactory,
                                  featureFlags>::commit()
    {
      filter = (VKLFilter)this->template getParam<int>("filter", filter);

      // Note: We fall back to the sampler object filter parameter if it is set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly
      //       if they set filter and want gradientFilter to be different.
      gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter", this->hasParam("filter") ? filter : gradientFilter);

      CALL_ISPC(Sampler_setFilters,
                this->getSh(),
                (ispc::VKLFilter)filter,
                (ispc::VKLFilter)gradientFilter);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline VKLFeatureFlagsInternal
    StructuredSampler<W,
                      IntervalIteratorFactory,
                      HitIteratorFactory,
                      featureFlags>::getFeatureFlags() const
    {
      if (this->isSpecConstsDisabled()) {
        return VKL_FEATURE_FLAG_ALL;
      }

      return featureFlags;
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSample(const vvec3fn<1> &objectCoordinates,
                                     vfloatn<1> &samples,
                                     unsigned int attributeIndex,
                                     const vfloatn<1> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTime(time[0]);
      CALL_ISPC(SharedStructuredVolume_sample_uniform_export,
                this->getSh(),
                &objectCoordinates,
                attributeIndex,
                &time,
                &samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSampleV(const vintn<W> &valid,
                                      const vvec3fn<W> &objectCoordinates,
                                      vfloatn<W> &samples,
                                      unsigned int attributeIndex,
                                      const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(SharedStructuredVolume_sample_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                attributeIndex,
                &time,
                &samples);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSampleN(unsigned int N,
                                      const vvec3fn<1> *objectCoordinates,
                                      float *samples,
                                      unsigned int attributeIndex,
                                      const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_sample_N_export,
                this->getSh(),
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
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeGradientV(const vintn<W> &valid,
                                        const vvec3fn<W> &objectCoordinates,
                                        vvec3fn<W> &gradients,
                                        unsigned int attributeIndex,
                                        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(SharedStructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                attributeIndex,
                time,
                &gradients);
    }

    template <int W,
              template <int>
              class IntervalIteratorFactory,
              template <int>
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeGradientN(unsigned int N,
                                        const vvec3fn<1> *objectCoordinates,
                                        vvec3fn<1> *gradients,
                                        unsigned int attributeIndex,
                                        const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_gradient_N_export,
                this->getSh(),
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
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSampleM(const vvec3fn<1> &objectCoordinates,
                                      float *samples,
                                      unsigned int M,
                                      const unsigned int *attributeIndices,
                                      const vfloatn<1> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTime(time[0]);
      CALL_ISPC(SharedStructuredVolume_sampleM_uniform_export,
                this->getSh(),
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
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSampleMV(const vintn<W> &valid,
                                       const vvec3fn<W> &objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const vfloatn<W> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTimes(valid, time);
      CALL_ISPC(SharedStructuredVolume_sampleM_export,
                static_cast<const int *>(valid),
                this->getSh(),
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
              class HitIteratorFactory,
              VKLFeatureFlagsInternal featureFlags>
    inline void StructuredSampler<
        W,
        IntervalIteratorFactory,
        HitIteratorFactory,
        featureFlags>::computeSampleMN(unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *times) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertAllValidTimes(N, times);
      CALL_ISPC(SharedStructuredVolume_sampleM_N_export,
                this->getSh(),
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
                          GridAcceleratorHitIteratorFactory,
                          VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME>;

    template <int W>
    using StructuredSphericalIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                DefaultIntervalIterator,
                                IntervalIteratorContext,
                                IntervalIteratorContext>;

    template <int W>
    using StructuredSphericalHitIterator =
        DefaultHitIterator<W, DefaultIntervalIterator<W>>;

    template <int W>
    using StructuredSphericalHitIteratorFactory =
        ConcreteIteratorFactory<W,
                                HitIterator,
                                StructuredSphericalHitIterator,
                                HitIteratorContext,
                                HitIteratorContext>;

    template <int W>
    using StructuredSphericalSampler =
        StructuredSampler<W,
                          StructuredSphericalIntervalIteratorFactory,
                          StructuredSphericalHitIteratorFactory,
                          VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME>;

  }  // namespace cpu_device
}  // namespace openvkl
