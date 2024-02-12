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

      bool supportsWidth(int width) override;

      int getNativeSIMDWidth() override;

      void commit() override;

      void commit(VKLObject object) override;
      void release(VKLObject object) override;

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

      VKLHitIteratorContext newHitIteratorContext(VKLSampler sampler) override;

      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLSampler newSampler(VKLVolume volume) override;

      VKLFeatureFlagsInternal getFeatureFlags(VKLSampler sampler) override;

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

      DeviceType getDeviceType() const override
      {
        return OPENVKL_DEVICE_TYPE_GPU;
      }

      AllocType getAllocationType(const void *ptr) const override;

      void *allocateSharedMemory(size_t numBytes,
                                 size_t alignment) const override;
      void freeSharedMemory(void *) const override;

      char *copyDeviceBufferToHost(size_t numItems,
                                   VKLDataType dataType,
                                   const void *source,
                                   size_t byteStride) override;

     private:
      template <int OW>
      typename std::enable_if<(OW < W), void>::type computeGradientAnyWidth(
          const int *valid,
          const VKLSampler *sampler,
          const vvec3fn<OW> &objectCoordinates,
          vvec3fn<OW> &gradients,
          unsigned int attributeIndex,
          const float *times);

      sycl::context syclContext;
      sycl::device syclDevice;
    };

  }  // namespace gpu_device
}  // namespace openvkl
