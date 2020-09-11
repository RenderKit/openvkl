// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/export_util.h"
#include "../iterator/DefaultIterator.h"
#include "../iterator/GridAcceleratorIterator.h"
#include "../sampler/Sampler.h"
#include "Sampler_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"
#include "StructuredRegularVolume.h"
#include "StructuredSphericalVolume.h"
#include "Volume_ispc.h"
#include "Sampler_ispc.h"
#include "openvkl/VKLFilter.h"

namespace openvkl {
  namespace ispc_driver {

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
    struct StructuredSampler : public SamplerBase<W, StructuredVolume>
    {
      StructuredSampler(StructuredVolume<W> *volume);
      ~StructuredSampler() override;

      void commit() override;

      // single attribute /////////////////////////////////////////////////////

      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples,
                         unsigned int attributeIndex) const override final;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples,
                          unsigned int attributeIndex) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples,
                          unsigned int attributeIndex) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients,
                            unsigned int attributeIndex) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients,
                            unsigned int attributeIndex) const override final;

      // multi-attribute //////////////////////////////////////////////////////

      void computeSampleM(
          const vvec3fn<1> &objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

      void computeSampleMV(
          const vintn<W> &valid,
          const vvec3fn<W> &objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

      void computeSampleMN(
          unsigned int N,
          const vvec3fn<1> *objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

      /////////////////////////////////////////////////////////////////////////

      const IteratorFactory<W, IntervalIterator> &getIntervalIteratorFactory()
          const override final;

      const IteratorFactory<W, HitIterator> &getHitIteratorFactory()
          const override final;

     protected:
      using Sampler<W>::ispcEquivalent;
      using SamplerBase<W, StructuredVolume>::volume;

      VKLFilter filter;
      VKLFilter gradientFilter;

     private:
      GridAcceleratorIntervalIteratorFactory<W> intervalIteratorFactoryRegular;
      GridAcceleratorHitIteratorFactory<W> hitIteratorFactoryRegular;

      StructuredSphericalIntervalIteratorFactory<W>
          intervalIteratorFactorSpherical;
      StructuredSphericalHitIteratorFactory<W> hitIteratorFactorySpherical;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline StructuredSampler<W>::StructuredSampler(StructuredVolume<W> *volume)
        : SamplerBase<W, StructuredVolume>(*volume),
          filter(volume->getFilter()),
          gradientFilter(volume->getGradientFilter())
    {
      assert(volume);
      ispcEquivalent = CALL_ISPC(StructuredSampler_create, 
                                 volume->getISPCEquivalent());
    }

    template <int W>
    inline StructuredSampler<W>::~StructuredSampler()
    {
      CALL_ISPC(StructuredSampler_destroy, ispcEquivalent);
      ispcEquivalent = nullptr;
    }

    template <int W>
    inline void StructuredSampler<W>::commit()
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

    template <int W>
    inline void StructuredSampler<W>::computeSample(
        const vvec3fn<1> &objectCoordinates,
        vfloatn<1> &samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_uniform_export,
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                &samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                &samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                attributeIndex,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                attributeIndex,
                &gradients);
    }

    template <int W>
    inline void StructuredSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_gradient_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                attributeIndex,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleM(
        const vvec3fn<1> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_uniform_export,
                ispcEquivalent,
                &objectCoordinates,
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleMV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_export,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleMN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_N_export,
                ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline const IteratorFactory<W, IntervalIterator>
        &StructuredSampler<W>::getIntervalIteratorFactory() const
    {
      if (dynamic_cast<const StructuredRegularVolume<W> *>(
              &this->getVolume())) {
        return intervalIteratorFactoryRegular;
      } else if (dynamic_cast<const StructuredSphericalVolume<W> *>(
                     &this->getVolume())) {
        return intervalIteratorFactorSpherical;
      } else {
        throw std::runtime_error("unsupported volume type on sampler");
      }
    }

    template <int W>
    inline const IteratorFactory<W, HitIterator>
        &StructuredSampler<W>::getHitIteratorFactory() const
    {
      if (dynamic_cast<const StructuredRegularVolume<W> *>(
              &this->getVolume())) {
        return hitIteratorFactoryRegular;
      } else if (dynamic_cast<const StructuredSphericalVolume<W> *>(
                     &this->getVolume())) {
        return hitIteratorFactorySpherical;
      } else {
        throw std::runtime_error("unsupported volume type on sampler");
      }
    }

  }  // namespace ispc_driver
}  // namespace openvkl
