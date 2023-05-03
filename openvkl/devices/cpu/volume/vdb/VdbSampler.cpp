// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "VdbGrid.h"
#include "VdbLeafAccessObserver.h"
#include "VdbSampler.h"
#include "VdbSampler_ispc.h"
#include "VdbVolume.h"
#include "openvkl/VKLDataType.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    VdbSampler<W>::VdbSampler(Device *device, VdbVolume<W> &volume)
        : AddStructShared<VdbSamplerBase<W>, ispc::VdbSamplerShared>(device,
                                                                     volume)
    {
      CALL_ISPC(VdbSampler_create,
                volume.getSh(),
                leafAccessObservers.getIE(),
                this->getSh());
    }

    template <int W>
    VdbSampler<W>::~VdbSampler()
    {
      CALL_ISPC(VdbSampler_destroy, this->getSh());
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

      CALL_ISPC(VdbSampler_set,
                this->getSh(),
                (ispc::VKLFilter)filter,
                (ispc::VKLFilter)gradientFilter,
                maxSamplingDepth);
    }

    template <int W>
    VKLFeatureFlags VdbSampler<W>::getFeatureFlags() const
    {
      VKLFeatureFlags ff = VKL_FEATURE_FLAG_NONE;

      if (this->getSh()->grid->dense) {
        ff |= VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME;
      } else {
        ff |= VKL_FEATURE_FLAG_VDB_VOLUME;
      }

      VKLFilter sampleFilter = this->getSh()->super.super.filter;

      switch (sampleFilter) {
      case VKL_FILTER_NEAREST:
        ff |= VKL_FEATURE_FLAG_SAMPLE_FILTER_NEAREST;
        break;

      case VKL_FILTER_TRILINEAR:
        ff |= VKL_FEATURE_FLAG_SAMPLE_FILTER_TRILINEAR;
        break;

      case VKL_FILTER_TRICUBIC:
        ff |= VKL_FEATURE_FLAG_SAMPLE_FILTER_TRICUBIC;
        break;

      default:
        assert(false);
      }

      VKLFilter gradientFilter = this->getSh()->super.super.gradientFilter;

      switch (gradientFilter) {
      case VKL_FILTER_NEAREST:
        ff |= VKL_FEATURE_FLAG_GRADIENT_FILTER_NEAREST;
        break;

      case VKL_FILTER_TRILINEAR:
        ff |= VKL_FEATURE_FLAG_GRADIENT_FILTER_TRILINEAR;
        break;

      case VKL_FILTER_TRICUBIC:
        ff |= VKL_FEATURE_FLAG_GRADIENT_FILTER_TRICUBIC;
        break;

      default:
        assert(false);
      }

      return ff;
    }

    template <int W>
    void VdbSampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                      vfloatn<1> &samples,
                                      unsigned int attributeIndex,
                                      const vfloatn<1> &time) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      assertValidTime(time[0]);
      CALL_ISPC(VdbSampler_computeSample_uniform,
                this->getSh(),
                &objectCoordinates,
                static_cast<const float *>(time),
                attributeIndex,
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
      assertValidTimes(valid, time);
      CALL_ISPC(VdbSampler_computeSample,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                static_cast<const float *>(time),
                attributeIndex,
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
      assertAllValidTimes(N, times);
      CALL_ISPC(VdbSampler_computeSample_stream,
                this->getSh(),
                N,
                (const ispc::vec3f *)objectCoordinates,
                times,
                attributeIndex,
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
      assertValidTimes(valid, time);
      CALL_ISPC(VdbSampler_computeGradient,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                static_cast<const float *>(time),
                attributeIndex,
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
      assertAllValidTimes(N, times);
      CALL_ISPC(VdbSampler_computeGradient_stream,
                this->getSh(),
                N,
                (const ispc::vec3f *)objectCoordinates,
                times,
                attributeIndex,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    inline void VdbSampler<W>::computeSampleM(
        const vvec3fn<1> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices,
        const vfloatn<1> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTime(time[0]);
      CALL_ISPC(VdbSampler_computeSampleM_uniform,
                this->getSh(),
                &objectCoordinates,
                static_cast<const float *>(time),
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void VdbSampler<W>::computeSampleMV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices,
        const vfloatn<W> &time) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertValidTimes(valid, time);
      CALL_ISPC(VdbSampler_computeSampleM,
                static_cast<const int *>(valid),
                this->getSh(),
                &objectCoordinates,
                static_cast<const float *>(time),
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void VdbSampler<W>::computeSampleMN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices,
        const float *times) const
    {
      assertValidAttributeIndices(volume, M, attributeIndices);
      assertAllValidTimes(N, times);
      CALL_ISPC(VdbSampler_computeSampleM_stream,
                this->getSh(),
                N,
                (ispc::vec3f *)objectCoordinates,
                times,
                M,
                attributeIndices,
                samples);
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

  }  // namespace cpu_device
}  // namespace openvkl
