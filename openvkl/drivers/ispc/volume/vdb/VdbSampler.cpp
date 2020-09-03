// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbSampler.h"
#include "VdbLeafAccessObserver.h"
#include "VdbSampler_ispc.h"
#include "VdbVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    VdbSampler<W>::VdbSampler(VdbVolume<W> &volume) 
      : SamplerBase<W, VdbVolume>(volume)
    {
      config.leafAccessObservers = leafAccessObservers.getIE();
    }

    template <int W>
    VdbSampler<W>::~VdbSampler() = default;

    template <int W>
    void VdbSampler<W>::commit()
    {
      const VdbSampleConfig &globalConfig = volume->getSampleConfig();
      config.filter = (VKLFilter)this->template getParam<int>(
          "filter", globalConfig.filter);

      // Note: We fall back to the sampler object filter parameter if it is set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly
      //       if they set filter and want gradientFilter to be different.
      config.gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter",
          this->hasParam("filter") ? config.filter
                                   : globalConfig.gradientFilter);

      config.maxSamplingDepth = this->template getParam<int>(
          "maxSamplingDepth", globalConfig.maxSamplingDepth);
    }

    template <int W>
    void VdbSampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                      vfloatn<1> &samples,
                                      unsigned int attributeIndex) const
    {
      assert(attributeIndex == 0);
      CALL_ISPC(VdbSampler_computeSample_uniform,
                volume->getGrid(),
                &config,
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
                volume->getGrid(),
                &config,
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
                volume->getGrid(),
                &config,
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
                volume->getGrid(),
                &config,
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
                volume->getGrid(),
                &config,
                N,
                (const ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    Observer<W> *VdbSampler<W>::newObserver(const char *type)
    {
      const std::string t(type);

      if (t == "LeafNodeAccess") {
        auto *obs = new VdbLeafAccessObserver<W>(*this, *volume->getGrid());
        return obs;
      }

      return Sampler<W>::newObserver(type);
    }

    template struct VdbSampler<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl

