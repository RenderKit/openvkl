// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/export_util.h"
#include "../../iterator/UnstructuredIterator.h"
#include "../../sampler/Sampler.h"
#include "../UnstructuredSamplerShared.h"
#include "AMRVolume.h"
#include "AMRVolume_ispc.h"
#include "Sampler_ispc.h"
#include "method_current_ispc.h"
#include "method_finest_ispc.h"
#include "method_octant_ispc.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    using AMRSamplerBase = SamplerBase<W,
                                       AMRVolume,
                                       UnstructuredIntervalIteratorFactory,
                                       UnstructuredHitIteratorFactory>;

    template <int W>
    struct AMRSampler : public AddStructShared<AMRSamplerBase<W>,
                                               ispc::UnstructuredSamplerShared>
    {
      AMRSampler(Device *, AMRVolume<W> &volume);
      ~AMRSampler() override;

      void commit() override;

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
                          const float *time) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients,
                            unsigned int attributeIndex,
                            const vfloatn<W> &time) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients,
                            unsigned int attributeIndex,
                            const float *time) const override final;

     protected:
      using AMRSamplerBase<W>::volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline AMRSampler<W>::AMRSampler(Device *device, AMRVolume<W> &volume)
        : AddStructShared<AMRSamplerBase<W>, ispc::UnstructuredSamplerShared>(
              device, volume)
    {
      CALL_ISPC(AMRSampler_create, volume.getSh(), this->getSh());
    }

    template <int W>
    inline AMRSampler<W>::~AMRSampler()
    {
      CALL_ISPC(AMRSampler_destroy, this->getSh());
    }

    template <int W>
    inline void AMRSampler<W>::commit()
    {
      const VKLAMRMethod amrMethod =
          (VKLAMRMethod)(this->template getParam<int>("method",
                                                      volume->getAMRMethod()));
      ispc::SamplerShared *ss = &(this->getSh()->super.super);
      if (amrMethod == VKL_AMR_CURRENT)
        CALL_ISPC(AMR_install_current, ss);
      else if (amrMethod == VKL_AMR_FINEST)
        CALL_ISPC(AMR_install_finest, ss);
      else if (amrMethod == VKL_AMR_OCTANT)
        CALL_ISPC(AMR_install_octant, ss);
      else
        throw std::runtime_error("AMRSampler: illegal method specified");
    }

    template <int W>
    inline VKLFeatureFlags AMRSampler<W>::getFeatureFlags() const
    {
      return VKL_FEATURE_FLAG_AMR_VOLUME;
    }

    template <int W>
    inline void AMRSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(AMRVolume_sample_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void AMRSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int attributeIndex,
        const float *time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, time);
      CALL_ISPC(Sampler_sample_N_export,
                this->getSh(),
                N,
                (ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    inline void AMRSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex,
        const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(valid, time);
      CALL_ISPC(AMRVolume_gradient_export,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void AMRSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients,
        unsigned int attributeIndex,
        const float *time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertAllValidTimes(N, time);
      CALL_ISPC(Sampler_gradient_N_export,
                this->getSh(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

  }  // namespace cpu_device
}  // namespace openvkl
