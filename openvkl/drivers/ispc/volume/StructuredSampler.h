// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/export_util.h"
#include "../sampler/Sampler.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"
#include "Volume_ispc.h"
#include "openvkl/VKLFilter.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredSampler : public Sampler<W>
    {
      StructuredSampler(const StructuredVolume<W> *volume);

      ~StructuredSampler() override = default;

      void commit() override;

      // single attribute /////////////////////////////////////////////////////

      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples,
                         unsigned int attributeIndex) const override final;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples,
                          unsigned int attributeIndex) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples,
                          unsigned int attributeIndex) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients,
                            unsigned int attributeIndex) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients,
                            unsigned int attributeIndex) const override final;

      // multi-attribute //////////////////////////////////////////////////////

      void computeSampleM(
          const vvec3fn<1> &objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

      void computeSampleMV(
          const vintn<W> &valid,
          const vvec3fn<W> &objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

      void computeSampleMN(
          unsigned int N,
          const vvec3fn<1> *objectCoordinates,
          float *samples,
          unsigned int M,
          const unsigned int *attributeIndices) const override final;

     protected:
      const StructuredVolume<W> *volume{nullptr};
      VKLFilter filter;
      VKLFilter gradientFilter;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline StructuredSampler<W>::StructuredSampler(
        const StructuredVolume<W> *volume)
        : volume(volume),
          filter(volume->getFilter()),
          gradientFilter(volume->getGradientFilter())
    {
      assert(volume);
    }

    template <int W>
    inline void StructuredSampler<W>::commit()
    {
      filter = (VKLFilter)this->template getParam<int>("filter", filter);

      // Note: We fall back to the sampler object filter parameter if it is set.
      //       This enables users to specify *only* the field filter override.
      //       This does mean that users must set the gradientFilter explicitly
      //       if they set filter and want gradientFilter to be different.
      gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter", this->hasParam("filter") ? filter : gradientFilter);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSample(
        const vvec3fn<1> &objectCoordinates,
        vfloatn<1> &samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_uniform_export,
                volume->getISPCEquivalent(),
                &objectCoordinates,
                (ispc::VKLFilter)filter,
                attributeIndex,
                &samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                (ispc::VKLFilter)filter,
                attributeIndex,
                &samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_sample_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::VKLFilter)filter,
                attributeIndex,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                (ispc::VKLFilter)gradientFilter,
                attributeIndex,
                &gradients);
    }

    template <int W>
    inline void StructuredSampler<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients,
        unsigned int attributeIndex) const
    {
      assert(attributeIndex < volume->getNumAttributes());
      CALL_ISPC(SharedStructuredVolume_gradient_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::VKLFilter)gradientFilter,
                attributeIndex,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleM(
        const vvec3fn<1> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_uniform_export,
                volume->getISPCEquivalent(),
                &objectCoordinates,
                (ispc::VKLFilter)filter,
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleMV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_export,
                static_cast<const int *>(valid),
                volume->getISPCEquivalent(),
                &objectCoordinates,
                (ispc::VKLFilter)filter,
                M,
                attributeIndices,
                samples);
    }

    template <int W>
    inline void StructuredSampler<W>::computeSampleMN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < M; i++)
        assert(attributeIndices[i] < volume->getNumAttributes());

      CALL_ISPC(SharedStructuredVolume_sampleM_N_export,
                volume->getISPCEquivalent(),
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::VKLFilter)filter,
                M,
                attributeIndices,
                samples);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
