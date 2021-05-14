// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/IteratorBase.h"
#include "../common/ManagedObject.h"
#include "../common/logging.h"
#include "../common/simd.h"
#include "Device.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/OnScopeExit.h"

#ifdef _WIN32
#include <process.h>  // for getpid
#endif

using namespace openvkl;

using namespace rkcommon::math;

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define THROW_IF_NULL(obj)                                            \
  if (obj == nullptr)                                                 \
  throw std::runtime_error(std::string("null ") + std::string(#obj) + \
                           std::string(" provided to ") + __FUNCTION__)

#define OPENVKL_CATCH_BEGIN_NO_DEVICE \
  {                                   \
    Device *deviceObj = nullptr;      \
    try {

#define OPENVKL_CATCH_BEGIN_SAFE(deviceSource)    \
  {                                               \
    THROW_IF_NULL(deviceSource);                  \
    Device *deviceObj = deviceFrom(deviceSource); \
    try {

#define OPENVKL_CATCH_BEGIN_UNSAFE(deviceSource)  \
  {                                               \
    assert(deviceSource != nullptr);              \
    Device *deviceObj = deviceFrom(deviceSource); \
    try {

#define OPENVKL_CATCH_END_NO_DEVICE(a)                                         \
  }                                                                            \
  catch (const std::bad_alloc &)                                               \
  {                                                                            \
    openvkl::handleError(deviceObj,                                            \
                         VKL_OUT_OF_MEMORY,                                    \
                         "Open VKL was unable to allocate memory");            \
    return a;                                                                  \
  }                                                                            \
  catch (const std::exception &e)                                              \
  {                                                                            \
    openvkl::handleError(deviceObj, VKL_UNKNOWN_ERROR, e.what());              \
    return a;                                                                  \
  }                                                                            \
  catch (...)                                                                  \
  {                                                                            \
    openvkl::handleError(                                                      \
        deviceObj, VKL_UNKNOWN_ERROR, "an unrecognized exception was caught"); \
    return a;                                                                  \
  }                                                                            \
  }

#define OPENVKL_CATCH_END(a) \
  assert(deviceObj);         \
  OPENVKL_CATCH_END_NO_DEVICE(a)

///////////////////////////////////////////////////////////////////////////////
// Device helpers /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline Device *deviceFrom(VKLObject object)
{
  return reinterpret_cast<openvkl::ManagedObject *>(object)->device.ptr;
}

inline Device *deviceFrom(VKLDevice device)
{
  return reinterpret_cast<Device *>(device);
}

inline void deviceAttach(Device *device, VKLObject object)
{
  auto mo    = reinterpret_cast<openvkl::ManagedObject *>(object);
  mo->device = device;
}

// helpers for iterators. note that iterators are not managed objects, and that
// we can't use std::is_convertible<> without augmenting the public (opaque)
// iterator types
template <typename IteratorT>
typename std::enable_if<
    std::is_same<IteratorT, IntervalIterator>::value ||
        std::is_same<IteratorT, IntervalIterator4>::value ||
        std::is_same<IteratorT, IntervalIterator8>::value ||
        std::is_same<IteratorT, IntervalIterator16>::value ||
        std::is_same<IteratorT, HitIterator>::value ||
        std::is_same<IteratorT, HitIterator4>::value ||
        std::is_same<IteratorT, HitIterator8>::value ||
        std::is_same<IteratorT, HitIterator16>::value,
    Device *>::type inline deviceFrom(IteratorT *it)
{
  return reinterpret_cast<openvkl::IteratorBase *>(it)->device;
}

template <typename IteratorT>
typename std::enable_if<
    std::is_same<IteratorT, IntervalIterator>::value ||
        std::is_same<IteratorT, IntervalIterator4>::value ||
        std::is_same<IteratorT, IntervalIterator8>::value ||
        std::is_same<IteratorT, IntervalIterator16>::value ||
        std::is_same<IteratorT, HitIterator>::value ||
        std::is_same<IteratorT, HitIterator4>::value ||
        std::is_same<IteratorT, HitIterator8>::value ||
        std::is_same<IteratorT, HitIterator16>::value,
    void>::type inline deviceAttach(Device *device, IteratorT *it)
{
  auto itObj    = reinterpret_cast<openvkl::IteratorBase *>(it);
  itObj->device = device;
}

///////////////////////////////////////////////////////////////////////////////
// Data ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLData vklNewData(VKLDevice device,
                              size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags,
                              size_t byteStride)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (!deviceObj->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  VKLData data = deviceObj->newData(
      numItems, dataType, source, dataCreationFlags, byteStride);
  deviceAttach(deviceObj, data);
  return data;
}
OPENVKL_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Observer ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLObserver vklNewVolumeObserver(VKLVolume volume, const char *type)
    OPENVKL_CATCH_BEGIN_SAFE(volume)
{
  THROW_IF_NULL(type);
  VKLObserver observer = deviceObj->newObserver(volume, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  deviceAttach(deviceObj, observer);
  return observer;
}
OPENVKL_CATCH_END(nullptr)

extern "C" VKLObserver vklNewSamplerObserver(VKLSampler sampler,
                                             const char *type)
    OPENVKL_CATCH_BEGIN_SAFE(sampler)
{
  THROW_IF_NULL(type);
  VKLObserver observer = deviceObj->newObserver(sampler, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  deviceAttach(deviceObj, observer);
  return observer;
}
OPENVKL_CATCH_END(nullptr)

extern "C" const void *vklMapObserver(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE(observer)
{
  const void *mapped = deviceObj->mapObserver(observer);
  return mapped;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklUnmapObserver(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE(observer)
{
  deviceObj->unmapObserver(observer);
}
OPENVKL_CATCH_END()

extern "C" VKLDataType vklGetObserverElementType(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE(observer)
{
  VKLDataType type = deviceObj->getObserverElementType(observer);
  return type;
}
OPENVKL_CATCH_END(VKL_UNKNOWN)

extern "C" size_t vklGetObserverElementSize(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE(observer)
{
  size_t size = deviceObj->getObserverElementSize(observer);
  return size;
}
OPENVKL_CATCH_END(0)

extern "C" size_t vklGetObserverNumElements(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE(observer)
{
  size_t numElements = deviceObj->getObserverNumElements(observer);
  return numElements;
}
OPENVKL_CATCH_END(0)

///////////////////////////////////////////////////////////////////////////////
// Device /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLDevice vklNewDevice(const char *deviceName)
    OPENVKL_CATCH_BEGIN_NO_DEVICE
{
  THROW_IF_NULL(deviceName);
  return (VKLDevice)Device::createDevice(deviceName);
}
OPENVKL_CATCH_END_NO_DEVICE(nullptr)

extern "C" void vklDeviceSetLogCallback(VKLDevice device,
                                        VKLLogCallback callback,
                                        void *userData)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (callback == nullptr) {
    deviceObj->logCallback = [](void *, const char *) {};
    deviceObj->logUserData = nullptr;
  } else {
    deviceObj->logCallback = callback;
    deviceObj->logUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetErrorCallback(VKLDevice device,
                                          VKLErrorCallback callback,
                                          void *userData)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (callback == nullptr) {
    deviceObj->errorCallback = [](void *, VKLError, const char *) {};
    deviceObj->errorUserData = nullptr;
  } else {
    deviceObj->errorCallback = callback;
    deviceObj->errorUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetInt(VKLDevice device, const char *name, int x)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(name);
  deviceObj->setParam<int>(name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetString(VKLDevice device,
                                   const char *name,
                                   const char *s)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(name);
  deviceObj->setParam<std::string>(name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklCommitDevice(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  deviceObj->commit();
}
OPENVKL_CATCH_END()

extern "C" VKLError vklDeviceGetLastErrorCode(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->lastErrorCode;
}
OPENVKL_CATCH_END(VKL_NO_ERROR)

extern "C" const char *vklDeviceGetLastErrorMsg(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->lastErrorMessage.c_str();
}
OPENVKL_CATCH_END(nullptr)

extern "C" int vklGetNativeSIMDWidth(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->getNativeSIMDWidth();
}
OPENVKL_CATCH_END(0)

extern "C" void vklCommit(VKLObject object) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  deviceObj->commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease(VKLObject object) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  deviceObj->release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklReleaseDevice(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  auto *object = (Device *)device;
  object->refDec();
}
OPENVKL_CATCH_END_NO_DEVICE()

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" size_t vklGetIntervalIteratorSize(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getIntervalIteratorSize1(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize4(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getIntervalIteratorSize4(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize8(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getIntervalIteratorSize8(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize16(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getIntervalIteratorSize16(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLIntervalIterator vklInitIntervalIterator(
    VKLSampler sampler,
    const vkl_vec3f *origin,
    const vkl_vec3f *direction,
    const vkl_range1f *tRange,
    VKLValueSelector valueSelector,
    void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  auto it = deviceObj->initIntervalIterator1(
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      valueSelector,
      buffer);
  deviceAttach(deviceObj, it);
  return it;
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitIntervalIteratorN(WIDTH)                        \
  extern "C" VKLIntervalIterator##WIDTH vklInitIntervalIterator##WIDTH( \
      const int *valid,                                                 \
      VKLSampler sampler,                                               \
      const vkl_vvec3f##WIDTH *origin,                                  \
      const vkl_vvec3f##WIDTH *direction,                               \
      const vkl_vrange1f##WIDTH *tRange,                                \
      VKLValueSelector valueSelector,                                   \
      void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)                 \
  {                                                                     \
    auto it = deviceObj->initIntervalIterator##WIDTH(                   \
        valid,                                                          \
        sampler,                                                        \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),              \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),           \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),            \
        valueSelector,                                                  \
        buffer);                                                        \
    deviceAttach(deviceObj, it);                                        \
    return it;                                                          \
  }                                                                     \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitIntervalIteratorN(4);
__define_vklInitIntervalIteratorN(8);
__define_vklInitIntervalIteratorN(16);

#undef __define_vklInitIntervalIteratorN

extern "C" int vklIterateInterval(VKLIntervalIterator iterator,
                                  VKLInterval *interval)
    OPENVKL_CATCH_BEGIN_UNSAFE(iterator)
{
  int result;
  deviceObj->iterateInterval1(
      iterator, reinterpret_cast<vVKLIntervalN<1> &>(*interval), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateIntervalN(WIDTH)                  \
  extern "C" void vklIterateInterval##WIDTH(                 \
      const int *valid,                                      \
      VKLIntervalIterator##WIDTH iterator,                   \
      VKLInterval##WIDTH *interval,                          \
      int *result) OPENVKL_CATCH_BEGIN_UNSAFE(iterator)      \
  {                                                          \
    deviceObj->iterateInterval##WIDTH(                       \
        valid,                                               \
        iterator,                                            \
        reinterpret_cast<vVKLIntervalN<WIDTH> &>(*interval), \
        result);                                             \
  }                                                          \
  OPENVKL_CATCH_END()

__define_vklIterateIntervalN(4);
__define_vklIterateIntervalN(8);
__define_vklIterateIntervalN(16);

#undef __define_vklIterateIntervalN

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" size_t vklGetHitIteratorSize(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getHitIteratorSize1(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize4(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getHitIteratorSize4(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize8(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getHitIteratorSize8(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize16(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  return deviceObj->getHitIteratorSize16(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLHitIterator vklInitHitIterator(VKLSampler sampler,
                                             const vkl_vec3f *origin,
                                             const vkl_vec3f *direction,
                                             const vkl_range1f *tRange,
                                             float time,
                                             VKLValueSelector valueSelector,
                                             void *buffer)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  auto it = deviceObj->initHitIterator1(
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      time,
      valueSelector,
      buffer);
  deviceAttach(deviceObj, it);
  return it;
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitHitIteratorN(WIDTH)                   \
  extern "C" VKLHitIterator##WIDTH vklInitHitIterator##WIDTH( \
      const int *valid,                                       \
      VKLSampler sampler,                                     \
      const vkl_vvec3f##WIDTH *origin,                        \
      const vkl_vvec3f##WIDTH *direction,                     \
      const vkl_vrange1f##WIDTH *tRange,                      \
      const float *times,                                     \
      VKLValueSelector valueSelector,                         \
      void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)       \
  {                                                           \
    auto it = deviceObj->initHitIterator##WIDTH(              \
        valid,                                                \
        sampler,                                              \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),    \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction), \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),  \
        times,                                                \
        valueSelector,                                        \
        buffer);                                              \
    deviceAttach(deviceObj, it);                              \
    return it;                                                \
  }                                                           \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitHitIteratorN(4);
__define_vklInitHitIteratorN(8);
__define_vklInitHitIteratorN(16);

#undef __define_vklInitHitIteratorN

extern "C" int vklIterateHit(VKLHitIterator iterator, VKLHit *hit)
    OPENVKL_CATCH_BEGIN_UNSAFE(iterator)
{
  int result;
  deviceObj->iterateHit1(
      iterator, reinterpret_cast<vVKLHitN<1> &>(*hit), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateHitN(WIDTH)                                       \
  extern "C" void vklIterateHit##WIDTH(const int *valid,                     \
                                       VKLHitIterator##WIDTH iterator,       \
                                       VKLHit##WIDTH *hit,                   \
                                       int *result)                          \
      OPENVKL_CATCH_BEGIN_UNSAFE(iterator)                                   \
  {                                                                          \
    deviceObj->iterateHit##WIDTH(                                            \
        valid, iterator, reinterpret_cast<vVKLHitN<WIDTH> &>(*hit), result); \
  }                                                                          \
  OPENVKL_CATCH_END()

__define_vklIterateHitN(4);
__define_vklIterateHitN(8);
__define_vklIterateHitN(16);

#undef __define_vklIterateHitN

///////////////////////////////////////////////////////////////////////////////
// Module /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLError vklLoadModule(const char *moduleName)
    OPENVKL_CATCH_BEGIN_NO_DEVICE
{
  THROW_IF_NULL(moduleName);
  return openvkl::loadLocalModule(moduleName);
}
OPENVKL_CATCH_END(VKL_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// Parameters /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklSetBool(VKLObject object, const char *name, int b)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setBool(object, name, static_cast<bool>(b));
}
OPENVKL_CATCH_END()

extern "C" void vklSetFloat(VKLObject object, const char *name, float x)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3f(VKLObject object,
                            const char *name,
                            float x,
                            float y,
                            float z) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetInt(VKLObject object, const char *name, int x)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3i(VKLObject object,
                            const char *name,
                            int x,
                            int y,
                            int z) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData(VKLObject object, const char *name, VKLData data)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString(VKLObject object, const char *name, const char *s)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr(VKLObject object, const char *name, void *v)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Value selector /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLValueSelector vklNewValueSelector(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE(volume)
{
  VKLValueSelector valueSelector = deviceObj->newValueSelector(volume);
  if (valueSelector == nullptr) {
    postLogMessage(deviceObj, VKL_LOG_ERROR)
        << "could not create value selector";
  }
  deviceAttach(deviceObj, valueSelector);
  return valueSelector;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                                          size_t numRanges,
                                          const vkl_range1f *ranges)
    OPENVKL_CATCH_BEGIN_UNSAFE(valueSelector)
{
  deviceObj->valueSelectorSetRanges(
      valueSelector,
      utility::ArrayView<const range1f>(
          reinterpret_cast<const range1f *>(ranges), numRanges));
}
OPENVKL_CATCH_END()

extern "C" void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                                          size_t numValues,
                                          const float *values)
    OPENVKL_CATCH_BEGIN_UNSAFE(valueSelector)
{
  deviceObj->valueSelectorSetValues(
      valueSelector,
      utility::ArrayView<const float>(reinterpret_cast<const float *>(values),
                                      numValues));
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Sampler ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLSampler vklNewSampler(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE(volume)
{
  VKLSampler sampler = deviceObj->newSampler(volume);
  if (sampler == nullptr) {
    postLogMessage(deviceObj, VKL_LOG_ERROR) << "could not create sampler";
  }
  deviceAttach(deviceObj, sampler);
  return sampler;
}
OPENVKL_CATCH_END(nullptr)

extern "C" float vklComputeSample(VKLSampler sampler,
                                  const vkl_vec3f *objectCoordinates,
                                  unsigned int attributeIndex,
                                  float time)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  float sample;
  deviceObj->computeSample1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      &sample,
      attributeIndex,
      &time);
  return sample;
}
OPENVKL_CATCH_END(rkcommon::math::nan)

#define __define_vklComputeSampleN(WIDTH)                             \
  extern "C" void vklComputeSample##WIDTH(                            \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeSample##WIDTH(                                  \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        samples,                                                      \
        attributeIndex,                                               \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeSampleN(4);
__define_vklComputeSampleN(8);
__define_vklComputeSampleN(16);

#undef __define_vklComputeSampleN

extern "C" void vklComputeSampleN(VKLSampler sampler,
                                  unsigned int N,
                                  const vkl_vec3f *objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex,
                                  const float *times)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeSampleN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

extern "C" void vklComputeSampleM(VKLSampler sampler,
                                  const vkl_vec3f *objectCoordinates,
                                  float *samples,
                                  unsigned int M,
                                  const unsigned int *attributeIndices,
                                  float time)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  deviceObj->computeSampleM1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      samples,
      M,
      attributeIndices,
      &time);
}
OPENVKL_CATCH_END()

#define __define_vklComputeSampleMN(WIDTH)                            \
  extern "C" void vklComputeSampleM##WIDTH(                           \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int M,                                                 \
      const unsigned int *attributeIndices,                           \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeSampleM##WIDTH(                                 \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        samples,                                                      \
        M,                                                            \
        attributeIndices,                                             \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeSampleMN(4);
__define_vklComputeSampleMN(8);
__define_vklComputeSampleMN(16);

#undef __define_vklComputeSampleMN

extern "C" void vklComputeSampleMN(VKLSampler sampler,
                                   unsigned int N,
                                   const vkl_vec3f *objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices,
                                   const float *times)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeSampleMN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      M,
      attributeIndices,
      times);
}
OPENVKL_CATCH_END()

extern "C" vkl_vec3f vklComputeGradient(VKLSampler sampler,
                                        const vkl_vec3f *objectCoordinates,
                                        unsigned int attributeIndex,
                                        float time)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  vkl_vec3f gradient;
  deviceObj->computeGradient1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      reinterpret_cast<vvec3fn<1> &>(gradient),
      attributeIndex,
      &time);
  return gradient;
}
OPENVKL_CATCH_END(vkl_vec3f{rkcommon::math::nan})

#define __define_vklComputeGradientN(WIDTH)                           \
  extern "C" void vklComputeGradient##WIDTH(                          \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      vkl_vvec3f##WIDTH *gradients,                                   \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeGradient##WIDTH(                                \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        reinterpret_cast<vvec3fn<WIDTH> &>(*gradients),               \
        attributeIndex,                                               \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeGradientN(4);
__define_vklComputeGradientN(8);
__define_vklComputeGradientN(16);

#undef __define_vklComputeGradientN

extern "C" void vklComputeGradientN(VKLSampler sampler,
                                    unsigned int N,
                                    const vkl_vec3f *objectCoordinates,
                                    vkl_vec3f *gradients,
                                    unsigned int attributeIndex,
                                    const float *times)
    OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeGradientN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      reinterpret_cast<vvec3fn<1> *>(gradients),
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Volume /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLVolume vklNewVolume(VKLDevice device, const char *type)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(type);

  if (!deviceObj->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  VKLVolume volume = deviceObj->newVolume(type);
  if (volume == nullptr) {
    postLogMessage(deviceObj, VKL_LOG_ERROR)
        << "could not create volume '" << type << "'";
  }
  deviceAttach(deviceObj, volume);
  return volume;
}
OPENVKL_CATCH_END(nullptr)

extern "C" vkl_box3f vklGetBoundingBox(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE(volume)
{
  const box3f result = deviceObj->getBoundingBox(volume);
  return reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END(vkl_box3f{rkcommon::math::nan})

extern "C" unsigned int vklGetNumAttributes(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE(volume)
{
  return deviceObj->getNumAttributes(volume);
}
OPENVKL_CATCH_END(0)

extern "C" vkl_range1f vklGetValueRange(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE(volume)
{
  const range1f result = deviceObj->getValueRange(volume);
  return reinterpret_cast<const vkl_range1f &>(result);
}
OPENVKL_CATCH_END(vkl_range1f{rkcommon::math::nan})
