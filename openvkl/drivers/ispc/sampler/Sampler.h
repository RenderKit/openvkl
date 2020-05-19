// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "../common/simd.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/vec.h"

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Sampler : public ManagedObject
    {
      // samplers can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      virtual void computeSample(const vvec3fn<1> &objectCoordinates,
                                 vfloatn<1> &samples) const = 0;

      virtual void computeSampleV(const vintn<W> &valid,
                                  const vvec3fn<W> &objectCoordinates,
                                  vfloatn<W> &samples) const = 0;

      virtual void computeSampleN(unsigned int N,
                                  const vvec3fn<1> *objectCoordinates,
                                  float *samples) const = 0;

      virtual void computeGradientV(const vintn<W> &valid,
                                    const vvec3fn<W> &objectCoordinates,
                                    vvec3fn<W> &gradients) const = 0;

      virtual void computeGradientN(unsigned int N,
                                    const vvec3fn<1> *objectCoordinates,
                                    vvec3fn<1> *gradients) const = 0;
    };

    /*
     * The DefaultSampler simply forwards sample calls to the volume type.
     */
    template <int W, class VolumeType>
    struct DefaultSampler : public Sampler<W>
    {
      explicit DefaultSampler(const VolumeType *volume);
      virtual ~DefaultSampler() override;

      // samplers can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      virtual void computeSample(const vvec3fn<1> &objectCoordinates,
                                 vfloatn<1> &samples) const override;

      virtual void computeSampleV(const vintn<W> &valid,
                                  const vvec3fn<W> &objectCoordinates,
                                  vfloatn<W> &samples) const override;

      virtual void computeSampleN(unsigned int N,
                                  const vvec3fn<1> *objectCoordinates,
                                  float *samples) const override;

      virtual void computeGradientV(const vintn<W> &valid,
                                    const vvec3fn<W> &objectCoordinates,
                                    vvec3fn<W> &gradients) const override;

      virtual void computeGradientN(unsigned int N,
                                    const vvec3fn<1> *objectCoordinates,
                                    vvec3fn<1> *gradients) const override;

     protected:
      const VolumeType *volume{nullptr};
    };

    template <int W, class VolumeType>
    inline DefaultSampler<W, VolumeType>::DefaultSampler(
        const VolumeType *volume)
        : volume(volume)
    {
      assert(volume);
    }

    template <int W, class VolumeType>
    inline DefaultSampler<W, VolumeType>::~DefaultSampler() = default;

    template <int W, class VolumeType>
    inline void DefaultSampler<W, VolumeType>::computeSample(
        const vvec3fn<1> &objectCoordinates, vfloatn<1> &samples) const
    {
      volume->computeSample(objectCoordinates, samples);
    }

    template <int W, class VolumeType>
    inline void DefaultSampler<W, VolumeType>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      volume->computeSampleV(valid, objectCoordinates, samples);
    }

    template <int W, class VolumeType>
    inline void DefaultSampler<W, VolumeType>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples) const
    {
      volume->computeSampleN(N, objectCoordinates, samples);
    }

    template <int W, class VolumeType>
    inline void DefaultSampler<W, VolumeType>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      volume->computeGradientV(valid, objectCoordinates, gradients);
    }

    template <int W, class VolumeType>
    inline void DefaultSampler<W, VolumeType>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients) const
    {
      volume->computeGradientN(N, objectCoordinates, gradients);
    }

  }  // namespace ispc_driver
}  // namespace openvkl

