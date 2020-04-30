// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbSampler.h"
#include "VdbSampler_ispc.h"
#include "VdbVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    VdbSampler<W>::VdbSampler(const VdbGrid *grid,
                              const VdbSampleConfig &defaultConfig)
        : grid(grid), config(defaultConfig)
    {
    }

    template <int W>
    VdbSampler<W>::~VdbSampler() = default;

    template <int W>
    void VdbSampler<W>::commit()
    {
      config.filter =
          (VKLFilter)this->template getParam<int>("filter", config.filter);
      config.maxSamplingDepth =
          this->template getParam<int>("maxSamplingDepth", config.maxSamplingDepth);
    }

    template <int W>
    void VdbSampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                      vfloatn<1> &samples) const
    {
      CALL_ISPC(VdbSampler_computeSample_uniform,
                this->grid,
                &this->config,
                &objectCoordinates,
                static_cast<float *>(samples));
    }

    template <int W>
    void VdbSampler<W>::computeSampleV(const vintn<W> &valid,
                                       const vvec3fn<W> &objectCoordinates,
                                       vfloatn<W> &samples) const
    {
      CALL_ISPC(VdbSampler_computeSample,
                static_cast<const int *>(valid),
                this->grid,
                &this->config,
                &objectCoordinates,
                static_cast<float *>(samples));
    }

    template <int W>
    void VdbSampler<W>::computeSampleN(unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples) const
    {
      CALL_ISPC(VdbSampler_computeSample_stream,
                this->grid,
                &this->config,
                N,
                (const ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    void VdbSampler<W>::computeGradientV(const vintn<W> &valid,
                                         const vvec3fn<W> &objectCoordinates,
                                         vvec3fn<W> &gradients) const
    {
      THROW_NOT_IMPLEMENTED;
    }

    template <int W>
    void VdbSampler<W>::computeGradientN(unsigned int N,
                                         const vvec3fn<1> *objectCoordinates,
                                         vvec3fn<1> *gradients) const
    {
      THROW_NOT_IMPLEMENTED;
    }

    template struct VdbSampler<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl

