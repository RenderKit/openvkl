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

      // Note: We fall back to the sampler object filter parameter if it set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly 
      //       if they set filter and want gradientFilter to be different.
      config.gradientFilter =
          (VKLFilter)this->template getParam<int>("gradientFilter", 
              this->hasParam("filter") ? config.filter : config.gradientFilter);

      config.maxSamplingDepth =
          this->template getParam<int>("maxSamplingDepth", config.maxSamplingDepth);
    }

    template <int W>
    void VdbSampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                      vfloatn<1> &samples,
                                      unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
      CALL_ISPC(VdbSampler_computeSample_uniform,
                this->grid,
                &this->config,
                &objectCoordinates,
                static_cast<float *>(samples));
    }

    template <int W>
    void VdbSampler<W>::computeSampleV(const vintn<W> &valid,
                                       const vvec3fn<W> &objectCoordinates,
                                       vfloatn<W> &samples,
                                       unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
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
                                       float *samples,
                                       unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
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
                                         vvec3fn<W> &gradients,
                                         unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
      CALL_ISPC(VdbSampler_computeGradient,
                static_cast<const int *>(valid),
                this->grid,
                &this->config,
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    void VdbSampler<W>::computeGradientN(unsigned int N,
                                         const vvec3fn<1> *objectCoordinates,
                                         vvec3fn<1> *gradients,
                                         unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
      CALL_ISPC(VdbSampler_computeGradient_stream,
                this->grid,
                &this->config,
                N,
                (const ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

    template struct VdbSampler<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl

