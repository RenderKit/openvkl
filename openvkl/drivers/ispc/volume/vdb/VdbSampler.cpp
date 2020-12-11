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
        : VdbSamplerBase<W>(volume)
    {
      ispcEquivalent = CALL_ISPC(VdbSampler_create,
                                 volume.getISPCEquivalent(),
                                 leafAccessObservers.getIE());
    }

    template <int W>
    VdbSampler<W>::~VdbSampler()
    {
      CALL_ISPC(VdbSampler_destroy, ispcEquivalent);
      ispcEquivalent = nullptr;
    }

    template <int W>
    void VdbSampler<W>::commit()
    {
      const VKLFilter filter = (VKLFilter)this->template getParam<int>(
          "filter", volume->getFilter());

      // Note: We fall back to the sampler object filter parameter if it is set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly
      //       if they set filter and want gradientFilter to be different.
      const VKLFilter gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter",
          this->hasParam("filter") ? filter : volume->getGradientFilter());

      const uint32_t maxSamplingDepth = this->template getParam<int>(
          "maxSamplingDepth", volume->getMaxSamplingDepth());

      const uint32_t maxIteratorDepth = this->template getParam<int>(
          "maxIteratorDepth", volume->getMaxIteratorDepth());

      CALL_ISPC(VdbSampler_set,
                ispcEquivalent,
                (ispc::VKLFilter)filter,
                (ispc::VKLFilter)gradientFilter,
                maxSamplingDepth,
                maxIteratorDepth);
    }

    template <int W>
    void VdbSampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                      vfloatn<1> &samples,
                                      unsigned int attributeIndex,
                                      const vfloatn<1> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(VdbSampler_computeSample_uniform,
                ispcEquivalent,
                &objectCoordinates,
                static_cast<float *>(samples));
    }

    template <int W>
    void VdbSampler<W>::computeSampleV(const vintn<W> &valid,
                                       const vvec3fn<W> &objectCoordinates,
                                       vfloatn<W> &samples,
                                       unsigned int attributeIndex,
                                       const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(VdbSampler_computeSample,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                static_cast<float *>(samples));
    }

    template <int W>
    void VdbSampler<W>::computeSampleN(unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples,
                                       unsigned int attributeIndex,
                                       const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(N, times);
      CALL_ISPC(VdbSampler_computeSample_stream,
                ispcEquivalent,
                N,
                (const ispc::vec3f *)objectCoordinates,
                samples);
    }

    template <int W>
    void VdbSampler<W>::computeGradientV(const vintn<W> &valid,
                                         const vvec3fn<W> &objectCoordinates,
                                         vvec3fn<W> &gradients,
                                         unsigned int attributeIndex,
                                         const vfloatn<W> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(time);
      CALL_ISPC(VdbSampler_computeGradient,
                static_cast<const int *>(valid),
                ispcEquivalent,
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    void VdbSampler<W>::computeGradientN(unsigned int N,
                                         const vvec3fn<1> *objectCoordinates,
                                         vvec3fn<1> *gradients,
                                         unsigned int attributeIndex,
                                         const float *times) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTimes(N, times);
      CALL_ISPC(VdbSampler_computeGradient_stream,
                ispcEquivalent,
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
