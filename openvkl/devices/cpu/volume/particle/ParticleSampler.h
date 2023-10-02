// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include "../../common/export_util.h"
#include "../../iterator/UnstructuredIterator.h"
#include "../../sampler/Sampler.h"
#include "../UnstructuredSamplerShared.h"
#include "ParticleVolume.h"
#include "ParticleVolume_ispc.h"
#include "Sampler_ispc.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    using ParticleSamplerBase = SamplerBase<W,
                                            ParticleVolume,
                                            UnstructuredIntervalIteratorFactory,
                                            UnstructuredHitIteratorFactory>;

    template <int W>
    struct ParticleSampler
        : public AddStructShared<ParticleSamplerBase<W>,
                                 ispc::UnstructuredSamplerShared>
    {
      ParticleSampler(Device *device, ParticleVolume<W> &volume);
      ~ParticleSampler();

      VKLFeatureFlagsInternal getFeatureFlags() const override;

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

     protected:
      using ParticleSamplerBase<W>::volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline ParticleSampler<W>::ParticleSampler(Device *device,
                                               ParticleVolume<W> &volume)
        : AddStructShared<ParticleSamplerBase<W>,
                          ispc::UnstructuredSamplerShared>(device, volume)
    {
      CALL_ISPC(VKLParticleSampler_Constructor, volume.getSh(), this->getSh());
    }

    template <int W>
    inline ParticleSampler<W>::~ParticleSampler()
    {
      CALL_ISPC(VKLParticleSampler_Destructor, this->getSh());
    }

    template <int W>
    inline VKLFeatureFlagsInternal ParticleSampler<W>::getFeatureFlags() const
    {
      if (this->isSpecConstsDisabled()) {
        return VKL_FEATURE_FLAG_ALL;
      }

      return VKL_FEATURE_FLAG_PARTICLE_VOLUME;
    }

    template <int W>
    inline void ParticleSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(VKLParticleVolume_sample_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void ParticleSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int attributeIndex,
        const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, times);
      CALL_ISPC(Sampler_sample_N_export,
                this->getSh(),
                N,
                (ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    inline void ParticleSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(VKLParticleVolume_gradient_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void ParticleSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients,
        unsigned int attributeIndex,
        const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, times);
      CALL_ISPC(Sampler_gradient_N_export,
                this->getSh(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

  }  // namespace cpu_device
}  // namespace openvkl
