// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/export_util.h"
#include "../../sampler/Sampler.h"
#include "ParticleVolume.h"
#include "ParticleVolume_ispc.h"
#include "Volume_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct ParticleSampler : public Sampler<W>
    {
      ParticleSampler(const ParticleVolume<W> *volume);

      ~ParticleSampler() override = default;

      void commit() override {}

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients) const override final;

     protected:
      const ParticleVolume<W> *volume{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline ParticleSampler<W>::ParticleSampler(const ParticleVolume<W> *volume)
        : volume(volume)
    {
      assert(volume);
    }

    template <int W>
    inline void ParticleSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      CALL_ISPC(VKLParticleVolume_sample_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void ParticleSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples) const
    {
      CALL_ISPC(Volume_sample_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    inline void ParticleSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      CALL_ISPC(VKLParticleVolume_gradient_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void ParticleSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients) const
    {
      CALL_ISPC(Volume_gradient_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
