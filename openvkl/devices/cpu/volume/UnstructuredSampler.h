// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include "../common/export_util.h"
#include "../iterator/UnstructuredIterator.h"
#include "../sampler/Sampler.h"
#include "Sampler_ispc.h"
#include "UnstructuredSamplerShared.h"
#include "UnstructuredVolume.h"
#include "UnstructuredVolume_ispc.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    using UnstructuredSamplerBase =
        SamplerBase<W,
                    UnstructuredVolume,
                    UnstructuredIntervalIteratorFactory,
                    UnstructuredHitIteratorFactory>;

    template <int W>
    struct UnstructuredSampler
        : public AddStructShared<UnstructuredSamplerBase<W>,
                                 ispc::UnstructuredSamplerShared>
    {
      UnstructuredSampler(Device *, UnstructuredVolume<W> &volume);
      ~UnstructuredSampler() override;

      VKLFeatureFlags getFeatureFlags() const override;

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

     private:
      using UnstructuredSamplerBase<W>::volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline UnstructuredSampler<W>::UnstructuredSampler(
        Device *device, UnstructuredVolume<W> &volume)
        : AddStructShared<UnstructuredSamplerBase<W>,
                          ispc::UnstructuredSamplerShared>(device, volume)
    {
      CALL_ISPC(
          VKLUnstructuredSampler_Constructor, volume.getSh(), this->getSh());
    }

    template <int W>
    inline UnstructuredSampler<W>::~UnstructuredSampler()
    {
      CALL_ISPC(VKLUnstructuredSampler_Destructor, this->getSh());
    }

    template <int W>
    inline VKLFeatureFlags UnstructuredSampler<W>::getFeatureFlags() const
    {
      if (this->isSpecConstsDisabled()) {
        return VKL_FEATURE_FLAG_ALL;
      }

      return VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME |
             volume->getCellTypeFeatureFlags();
    }

    template <int W>
    inline void UnstructuredSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(VKLUnstructuredVolume_sample_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void UnstructuredSampler<W>::computeSampleN(
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
    inline void UnstructuredSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(VKLUnstructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void UnstructuredSampler<W>::computeGradientN(
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
