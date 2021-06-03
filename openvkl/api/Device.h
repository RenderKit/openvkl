// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <string>
#include "../common/VKLCommon.h"
#include "../common/simd.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/memory/IntrusivePtr.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/ParameterizedObject.h"

#ifdef NDEBUG
#define LOG_LEVEL_DEFAULT VKL_LOG_INFO
#else
#define LOG_LEVEL_DEFAULT VKL_LOG_DEBUG
#endif

using namespace rkcommon;

namespace openvkl {
  namespace api {

    struct OPENVKL_CORE_INTERFACE Device
        : public rkcommon::memory::RefCountedObject,
          public rkcommon::utility::ParameterizedObject
    {
      Device();
      virtual ~Device() override = default;

      static Device *createDevice(const std::string &deviceName);

      // error tracking
      VKLError lastErrorCode       = VKL_NO_ERROR;
      std::string lastErrorMessage = "no error";

      virtual bool supportsWidth(int width) = 0;

      virtual int getNativeSIMDWidth() = 0;

      virtual void commit();
      bool isCommitted();

      virtual void commit(VKLObject object)  = 0;
      virtual void release(VKLObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Device parameters (updated on commit()) //////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLLogLevel logLevel = LOG_LEVEL_DEFAULT;

      std::function<void(void *, const char *)> logCallback{
          [](void *, const char *) {}};
      void *logUserData{nullptr};

      std::function<void(void *, VKLError, const char *)> errorCallback{
          [](void *, VKLError, const char *) {}};
      void *errorUserData{nullptr};

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLData newData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags,
                              size_t byteStride) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Observer /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLObserver newObserver(VKLVolume volume, const char *type)   = 0;
      virtual VKLObserver newObserver(VKLSampler sampler, const char *type) = 0;
      virtual const void *mapObserver(VKLObserver observer)                 = 0;
      virtual void unmapObserver(VKLObserver observer)                      = 0;
      virtual VKLDataType getObserverElementType(
          VKLObserver observer) const                                   = 0;
      virtual size_t getObserverElementSize(VKLObserver observer) const = 0;
      virtual size_t getObserverNumElements(VKLObserver observer) const = 0;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLIntervalIteratorContext newIntervalIteratorContext(
          VKLSampler sampler, unsigned int attributeIndex) = 0;

#define __define_getIntervalIteratorSizeN(WIDTH) \
  virtual size_t getIntervalIteratorSize##WIDTH( \
      VKLIntervalIteratorContext context) const = 0;

      __define_getIntervalIteratorSizeN(1);
      __define_getIntervalIteratorSizeN(4);
      __define_getIntervalIteratorSizeN(8);
      __define_getIntervalIteratorSizeN(16);

#undef __define_getIntervalIteratorSizeN

      virtual VKLIntervalIterator initIntervalIterator1(
          VKLIntervalIteratorContext context,
          const vvec3fn<1> &origin,
          const vvec3fn<1> &direction,
          const vrange1fn<1> &tRange,
          void *buffer) const = 0;

#define __define_initIntervalIteratorN(WIDTH)                     \
  virtual VKLIntervalIterator##WIDTH initIntervalIterator##WIDTH( \
      const int *valid,                                           \
      VKLIntervalIteratorContext context,                         \
      const vvec3fn<WIDTH> &origin,                               \
      const vvec3fn<WIDTH> &direction,                            \
      const vrange1fn<WIDTH> &tRange,                             \
      void *buffer) const = 0;

      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

      virtual void iterateInterval1(VKLIntervalIterator iterator,
                                    vVKLIntervalN<1> &interval,
                                    int *result) const = 0;

#define __define_iterateIntervalN(WIDTH)                                   \
  virtual void iterateInterval##WIDTH(const int *valid,                    \
                                      VKLIntervalIterator##WIDTH iterator, \
                                      vVKLIntervalN<WIDTH> &interval,      \
                                      int *result) const = 0;

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLHitIteratorContext newHitIteratorContext(
          VKLSampler sampler, unsigned int attributeIndex) = 0;

#define __define_getHitIteratorSizeN(WIDTH)                               \
  virtual size_t getHitIteratorSize##WIDTH(VKLHitIteratorContext context) \
      const = 0;

      __define_getHitIteratorSizeN(1);
      __define_getHitIteratorSizeN(4);
      __define_getHitIteratorSizeN(8);
      __define_getHitIteratorSizeN(16);

#undef __define_getHitIteratorSizeN

      virtual VKLHitIterator initHitIterator1(VKLHitIteratorContext context,
                                              const vvec3fn<1> &origin,
                                              const vvec3fn<1> &direction,
                                              const vrange1fn<1> &tRange,
                                              float time,
                                              void *buffer) const = 0;

#define __define_initHitIteratorN(WIDTH)                \
  virtual VKLHitIterator##WIDTH initHitIterator##WIDTH( \
      const int *valid,                                 \
      VKLHitIteratorContext context,                    \
      const vvec3fn<WIDTH> &origin,                     \
      const vvec3fn<WIDTH> &direction,                  \
      const vrange1fn<WIDTH> &tRange,                   \
      const float *times,                               \
      void *buffer) const = 0;

      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

      virtual void iterateHit1(VKLHitIterator iterator,
                               vVKLHitN<1> &hit,
                               int *result) const = 0;

#define __define_iterateHitN(WIDTH)                              \
  virtual void iterateHit##WIDTH(const int *valid,               \
                                 VKLHitIterator##WIDTH iterator, \
                                 vVKLHitN<WIDTH> &hit,           \
                                 int *result) const = 0;

      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void setBool(VKLObject object,
                           const char *name,
                           const bool b)                                    = 0;
      virtual void set1f(VKLObject object, const char *name, const float x) = 0;
      virtual void set1i(VKLObject object, const char *name, const int x)   = 0;
      virtual void setVec3f(VKLObject object,
                            const char *name,
                            const math::vec3f &v)                           = 0;
      virtual void setVec3i(VKLObject object,
                            const char *name,
                            const math::vec3i &v)                           = 0;
      virtual void setObject(VKLObject object,
                             const char *name,
                             VKLObject setObject)                           = 0;
      virtual void setString(VKLObject object,
                             const char *name,
                             const std::string &s)                          = 0;
      virtual void setVoidPtr(VKLObject object, const char *name, void *v)  = 0;

      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLSampler newSampler(VKLVolume volume) = 0;

#define __define_computeSampleN(WIDTH)                                       \
  virtual void computeSample##WIDTH(const int *valid,                        \
                                    VKLSampler sampler,                      \
                                    const vvec3fn<WIDTH> &objectCoordinates, \
                                    float *samples,                          \
                                    unsigned int attributeIndex,             \
                                    const float *times) = 0;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      virtual void computeSampleN(VKLSampler sampler,
                                  unsigned int N,
                                  const vvec3fn<1> *objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex,
                                  const float *times) = 0;

#define __define_computeSampleMN(WIDTH)                                       \
  virtual void computeSampleM##WIDTH(const int *valid,                        \
                                     VKLSampler sampler,                      \
                                     const vvec3fn<WIDTH> &objectCoordinates, \
                                     float *samples,                          \
                                     unsigned int M,                          \
                                     const unsigned int *attributeIndices,    \
                                     const float *times) = 0;

      __define_computeSampleMN(1);
      __define_computeSampleMN(4);
      __define_computeSampleMN(8);
      __define_computeSampleMN(16);

#undef __define_computeSampleMN

      virtual void computeSampleMN(VKLSampler sampler,
                                   unsigned int N,
                                   const vvec3fn<1> *objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices,
                                   const float *times) = 0;

#define __define_computeGradientN(WIDTH)                                       \
  virtual void computeGradient##WIDTH(const int *valid,                        \
                                      VKLSampler sampler,                      \
                                      const vvec3fn<WIDTH> &objectCoordinates, \
                                      vvec3fn<WIDTH> &gradients,               \
                                      unsigned int attributeIndex,             \
                                      const float *times) = 0;

      __define_computeGradientN(1);
      __define_computeGradientN(4);
      __define_computeGradientN(8);
      __define_computeGradientN(16);

#undef __define_computeGradientN

      virtual void computeGradientN(VKLSampler sampler,
                                    unsigned int N,
                                    const vvec3fn<1> *objectCoordinates,
                                    vvec3fn<1> *gradients,
                                    unsigned int attributeIndex,
                                    const float *times) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLVolume newVolume(const char *type) = 0;

      virtual math::box3f getBoundingBox(VKLVolume volume) = 0;

      virtual unsigned int getNumAttributes(VKLVolume volume) = 0;

      virtual math::range1f getValueRange(VKLVolume volume,
                                          unsigned int attributeIndex) = 0;

     private:
      bool committed = false;
    };

#define VKL_REGISTER_DEVICE(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::api::Device, device, InternalClass, external_name)

  }  // namespace api
}  // namespace openvkl
