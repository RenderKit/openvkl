// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <sycl/sycl.hpp>
#include "../../cpu/common/align.h"
#include "../../cpu/iterator/Iterator.h"
#include "../../cpu/sampler/Sampler.h"
#include "AddDeviceAPIs.h"

namespace openvkl {
  namespace gpu_device {

    /*
     * Type alias to make enable_if more concise (we cannot rely on C++14).
     */
    template <bool C, class T = void>
    using EnableIf = typename std::enable_if<C, T>::type;

    template <int W>
    struct GPUDevice : public AddDeviceAPIs
    {
      GPUDevice() = default;
      ~GPUDevice() override;

      bool supportsWidth(int width) override;

      int getNativeSIMDWidth() override;

      void commit() override;

      // TODO: to be removed
      void commit(VKLObject object) override;
      void release(VKLObject object) override;

      void commit(APIObject object) override;
      void release(APIObject object) override;

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLData newData(size_t numItems,
                      VKLDataType dataType,
                      const void *source,
                      VKLDataCreationFlags dataCreationFlags,
                      size_t byteStride) override;

      /////////////////////////////////////////////////////////////////////////
      // Observer /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLObserver newVolumeObserver(VKLVolume volume, const char *type) override
      {
        throw std::runtime_error("not implemented");
      }
      VKLObserver newSamplerObserver(VKLSampler sampler,
                                     const char *type) override
      {
        throw std::runtime_error("not implemented");
      }
      const void *mapObserver(VKLObserver observer) override
      {
        throw std::runtime_error("not implemented");
      }
      void unmapObserver(VKLObserver observer) override
      {
        throw std::runtime_error("not implemented");
      }
      VKLDataType getObserverElementType(VKLObserver observer) const override
      {
        throw std::runtime_error("not implemented");
      }
      size_t getObserverElementSize(VKLObserver observer) const override
      {
        throw std::runtime_error("not implemented");
      }
      size_t getObserverNumElements(VKLObserver observer) const override
      {
        throw std::runtime_error("not implemented");
      }

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLIntervalIteratorContext newIntervalIteratorContext(
          VKLSampler sampler) override;

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLHitIteratorContext newHitIteratorContext(VKLSampler sampler) override
      {
        throw std::runtime_error("not implemented");
      }

      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLSampler newSampler(VKLVolume volume) override;

      void computeSampleM1(const int *valid,
                           const VKLSampler *sampler,
                           const vvec3fn<1> &objectCoordinates,
                           float *samples,
                           unsigned int M,
                           const unsigned int *attributeIndices,
                           const float *times) override;

      void computeGradient1(const int *valid,
                            const VKLSampler *sampler,
                            const vvec3fn<1> &objectCoordinates,
                            vvec3fn<1> &gradients,
                            unsigned int attributeIndex,
                            const float *times) override;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLVolume newVolume(const char *type) override;

      box3f getBoundingBox(VKLVolume volume) override;

      unsigned int getNumAttributes(VKLVolume volume) override;

      range1f getValueRange(VKLVolume volume,
                            unsigned int attributeIndex) override;

      /////////////////////////////////////////////////////////////////////////
      // Hardware facilities //////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////
      virtual void *getContext() const override
      {
        return context;
      };
      openvkl::api::memstate *allocateBytes(size_t numByte) const override;
      void freeMemState(openvkl::api::memstate *) const override;

     private:
      template <int OW>
      typename std::enable_if<(OW < W), void>::type computeGradientAnyWidth(
          const int *valid,
          const VKLSampler *sampler,
          const vvec3fn<OW> &objectCoordinates,
          vvec3fn<OW> &gradients,
          unsigned int attributeIndex,
          const float *times);

      void *context{nullptr};
    };

  }  // namespace gpu_device
}  // namespace openvkl
