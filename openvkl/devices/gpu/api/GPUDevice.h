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
          VKLSampler sampler) override
      {
        throw std::runtime_error("not implemented");
      }

      size_t getIntervalIteratorSize1(
          const VKLIntervalIteratorContext *context) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_getIntervalIteratorSizeN(WIDTH)                \
  size_t getIntervalIteratorSize##WIDTH(                        \
      const VKLIntervalIteratorContext *context) const override \
  {                                                             \
    throw std::runtime_error("not implemented");                \
  }

      __define_getIntervalIteratorSizeN(4);
      __define_getIntervalIteratorSizeN(8);
      __define_getIntervalIteratorSizeN(16);

#undef __define_getIntervalIteratorSizeN

      VKLIntervalIterator initIntervalIterator1(
          const VKLIntervalIteratorContext *context,
          const vvec3fn<1> &origin,
          const vvec3fn<1> &direction,
          const vrange1fn<1> &tRange,
          float time,
          void *buffer) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_initIntervalIteratorN(WIDTH)             \
  VKLIntervalIterator##WIDTH initIntervalIterator##WIDTH( \
      const int *valid,                                   \
      const VKLIntervalIteratorContext *context,          \
      const vvec3fn<WIDTH> &origin,                       \
      const vvec3fn<WIDTH> &direction,                    \
      const vrange1fn<WIDTH> &tRange,                     \
      const float *times,                                 \
      void *buffer) const override                        \
  {                                                       \
    throw std::runtime_error("not implemented");          \
  }

      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

      void iterateInterval1(const VKLIntervalIterator iterator,
                            vVKLIntervalN<1> &interval,
                            int *result) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_iterateIntervalN(WIDTH)                           \
  void iterateInterval##WIDTH(const int *valid,                    \
                              VKLIntervalIterator##WIDTH iterator, \
                              vVKLIntervalN<WIDTH> &interval,      \
                              int *result) const override          \
  {                                                                \
    throw std::runtime_error("not implemented");                   \
  }

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLHitIteratorContext newHitIteratorContext(VKLSampler sampler) override
      {
        throw std::runtime_error("not implemented");
      }

      size_t getHitIteratorSize1(
          const VKLHitIteratorContext *context) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_getHitIteratorSizeN(WIDTH)                              \
  size_t getHitIteratorSize##WIDTH(const VKLHitIteratorContext *context) \
      const override                                                     \
  {                                                                      \
    throw std::runtime_error("not implemented");                         \
  }

      __define_getHitIteratorSizeN(4);
      __define_getHitIteratorSizeN(8);
      __define_getHitIteratorSizeN(16);

#undef __define_getIntervalIteratorSizeN

      VKLHitIterator initHitIterator1(const VKLHitIteratorContext *context,
                                      const vvec3fn<1> &origin,
                                      const vvec3fn<1> &direction,
                                      const vrange1fn<1> &tRange,
                                      float time,
                                      void *buffer) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_initHitIteratorN(WIDTH)         \
  VKLHitIterator##WIDTH initHitIterator##WIDTH(  \
      const int *valid,                          \
      const VKLHitIteratorContext *context,      \
      const vvec3fn<WIDTH> &origin,              \
      const vvec3fn<WIDTH> &direction,           \
      const vrange1fn<WIDTH> &tRange,            \
      const float *times,                        \
      void *buffer) const override               \
  {                                              \
    throw std::runtime_error("not implemented"); \
  }

      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

      void iterateHit1(VKLHitIterator iterator,
                       vVKLHitN<1> &hit,
                       int *result) const override
      {
        throw std::runtime_error("not implemented");
      }

#define __define_iterateHitN(WIDTH)                      \
  void iterateHit##WIDTH(const int *valid,               \
                         VKLHitIterator##WIDTH iterator, \
                         vVKLHitN<WIDTH> &hit,           \
                         int *result) const override     \
  {                                                      \
    throw std::runtime_error("not implemented");         \
  }

      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN

      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLSampler newSampler(VKLVolume volume) override;

      void computeSample1(const int *valid,
                          const VKLSampler *sampler,
                          const vvec3fn<1> &objectCoordinates,
                          float *samples,
                          unsigned int attributeIndex,
                          const float *times) override;

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
      virtual void *getContext() const override { return context; };
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
