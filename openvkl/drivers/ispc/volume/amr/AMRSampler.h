// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/export_util.h"
#include "../../iterator/DefaultIterator.h"
#include "../../iterator/Iterator.h"
#include "../../sampler/Sampler.h"
#include "AMRVolume.h"
#include "AMRVolume_ispc.h"
#include "Volume_ispc.h"
#include "Sampler_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    using AmrIntervalIteratorFactory =
        ConcreteIteratorFactory<W, IntervalIterator, DefaultIntervalIterator>;

    template <int W>
    using AmrHitIterator = DefaultHitIterator<W, DefaultIntervalIterator<W>>;

    template <int W>
    using AmrHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, AmrHitIterator>;

    template <int W>
    struct AMRSampler : public SamplerBase<W, AMRVolume>
    {
      AMRSampler(AMRVolume<W> *volume);
      ~AMRSampler() override;

      void commit() override {}

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

      const IteratorFactory<W, IntervalIterator> &getIntervalIteratorFactory()
          const override final;

      const IteratorFactory<W, HitIterator> &getHitIteratorFactory()
          const override final;

     protected:
      using Sampler<W>::ispcEquivalent;
      using SamplerBase<W, AMRVolume>::volume;

      AmrIntervalIteratorFactory<W> intervalIteratorFactory;
      AmrHitIteratorFactory<W> hitIteratorFactory;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline AMRSampler<W>::AMRSampler(AMRVolume<W> *volume)
        : SamplerBase<W, AMRVolume>(*volume)
    {
      assert(volume);
      ispcEquivalent = CALL_ISPC(Sampler_create, volume->getISPCEquivalent());
    }

    template <int W>
    inline AMRSampler<W>::~AMRSampler()
    {
      CALL_ISPC(Sampler_destroy, ispcEquivalent);
      ispcEquivalent = nullptr;
    }

    template <int W>
    inline void AMRSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(AMRVolume_sample_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void AMRSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(Volume_sample_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    inline void AMRSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(AMRVolume_gradient_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void AMRSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(Volume_gradient_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    inline const IteratorFactory<W, IntervalIterator>
        &AMRSampler<W>::getIntervalIteratorFactory() const
    {
      return intervalIteratorFactory;
    }

    template <int W>
    inline const IteratorFactory<W, HitIterator>
        &AMRSampler<W>::getHitIteratorFactory() const
    {
      return hitIteratorFactory;
    }

  }  // namespace ispc_driver
}  // namespace openvkl
